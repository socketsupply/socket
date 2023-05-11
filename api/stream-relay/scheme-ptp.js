import { sodium } from '../crypto.js'
import { Packet, validatePacket, sha256 } from './packets.js'

const networkIdentities = {}
let defaultIdentity

const MAX_GROUP_MEMBERS = 6
const MAX_GROUP_LABEL_LENGTH = 40
// group member max is because we need to limit
// the identity-group message size, serialized as
// JSON, to 1024b (1 packet), including the group
// group (if any).
//
// group public-key and signature add 146 chars
// (44 chars for the public key, 88 chars for the
// signature, 14 chars for the properties `"p":`
// and `"s":`, 4 surrounding " quote chars, and
// the 2 commas).
//
// if a group label is included, `"l":` adds 7 chars
// plus the length of the label (max 40), bringing the
// JSON size so far up to max of 193 chars.
//
// each member adds 138 chars to the JSON (44 chars
// for the public-key, 88 chars for the signature, and
// 6 chars for the surrounding " quote chars, the :,
// and the comma); 6 * 138 = 828 chars
//
// note: even if label is omitted, that only saves
// ~47 chars, so not enough space for a 7th member
//
// total JSON length comes to max of 193 + 828 = ~1021
// chars, fitting safely within the 1024b packet limit
//
// Example:
//
// JSON.stringify({
//   p: "sQvh9X8teG0diekqrKT+SPNJdzbgl+sczIkGvOyjG08=",
//   s: "qIwOzvPGG4xob4OkSTzFfiK+KV9lfNuyfOLqoEhat0/Z6CKcYjBu6UJClEqDamCGsTQnRRlw6UQZKOmX09oDCA==",
//   l: "abcdefghijklmnopqrstuvwxyzabcdefghijklm",
//   "/gR05WcN26+Y0tWdlH2QiVPiWl9X+Q4OlzslcxkgN/0=": "gAHK6/+VcZuKatrRCFLLABnJV0njv63GyrSXRnwQ/JC74yDuMid1sF/Jc7p4gAs9A/9VukMPc8Ol6AeNJRhMDQ==",
//   "OcY+8h1alUzDEl9Bzflk4IM1XdwyR/KQspGTRST46JQ=": "QyyERbhr7bZQqtGLWkVgWnGanp7cluVI2WWWqHM+7lbzQrQDFVJRLwQwwOonq98xAT+RGNSUtbQXL6qR01FlAw==",
//   "jzusOVZnahzj+m0NTOhcCbDk3ccmY3gecOyjb6FPFQ0=": "hcThmLI5JLaEC6nFRp4Fg/MOz7RsirUaCWvZ0HsaYMW8zaikWQwGAHWg2gRorMIwiSAKiSnMGCKiQwNZgFDSAg==",
//   "nG9MiUNd8pajZXGq+EB2KeOoG0SM0AviWYZAta66Lak=": "bmnOytrnfYwe8nROnTLXLObHlErE+DzZ1KDqtIWwH2LE18A2Wr2yaf9KK1VECRlUCvnuVNhVODODwWP60bG8Dw==",
//   "s63v/dLVFoldbV2iU8biC5P0HoZlZy2GWV0VRcCYN/0=": "dxWjJBVBegTgqM6mchUA3LKze67zjU3YatlFY96x+Zs/Fw9XuvdFiHFfa2yi5m8oPeNeqJVx5nSNqwldrSWHBg==",
//   "uELxa+THWLVNMvBNq/LFz88V14K4ACVJU9f1Db+oI6g=": "U6ejxPukF8viU5o2VMl8EIFGeuAaGzO/FI7Z2CqKcfF/MLoT4NeKb3I0NVX9zG4EO6cUozt+EohCNNL5E/QNAQ=="
// }).length;
// 1021

export class PacketIdentityGroup extends Packet {
  static type = 9
  // TODO: probably want to apply custom behavior
  // to lifecycle of these packets, expiring quicker/etc
  constructor (args) {
    super({ type: PacketIdentityGroup.type, ...args })

    validatePacket(args, {
      message: { required: true, type: 'string' }
      // TODO: more to validate?
    })
  }
}

async function broadcastGroups () {
  // lookup any local group identities (skipping default identity)
  const groupIdentities = lookupNetworkIdentities().slice(1)

  const sends = groupIdentities
    .map(group => {
      const { data, err } = buildIdentityGroupMessage(group.publicKey, group.label)
      // TODO: potentially log more verbose error here
      if (err) return undefined

      return new PacketIdentityGroup({
        message: JSON.stringify(data)
      })
    })
    .filter(Boolean)
    .map(packet => Packet.encode(packet).then(data =>
      this.mcast(data, data.packetId, data.clusterId)
    ))

  if (sends.length === 0) return false

  try {
    await Promise.all(sends)
    return true
  } catch (e) {
    return false
  }
}

export class PTP {
  static init (peer, config, peerType = 'local') {
    if (peerType === 'local') {
      config.keys = config.keys ?? generateNetworkIdentity()
      config.peerId = formatPeerId(config.keys.publicKey)

      // register local identity (full keypair)
      registerNetworkIdentity(config.keys)
    } else {
      // register remote identity (public-key only)
      registerNetworkIdentity({ publicKey: config.peerId })
    }

    peer.prepareMessage = prepareNetworkMessage

    // groups broadcasted at least a day ago (or never)?
    peer.onInterval = async () => {
      const oneDayAgo = Date.now() - (24 * 60 * 60 * 1000)
      if (peer.config.lastGroupBroadcast <= oneDayAgo) {
        if (await broadcastGroups()) {
          peer.config.lastGroupBroadcast = Date.now()
        }
      }
    }

    peer.onData = (packet, port, address, data) => {
      if (packet.type !== PacketIdentityGroup.type) return
      // notification received of an identity-group broadcast message, from another peer
      // register remote identity group (if valid)
      // TODO: potentially log errors at more verbose log level
      try {
        registerRemoteIdentityGroup(JSON.parse(data))
      } catch {}
    }

    peer.predicateOnPacket = async (packet, port, address) => {
      // lookup identities we can decrypt this specific packet with (if any)
      const identities = lookupNetworkIdentities(packet.to)

      for (const identity of identities) {
        // local identity (able to decrypt)?
        if (identity.privateKey && peer.onPacket) {
          // multipart series of packets?
          if (packet.index > -1) {
            const p = await peer.cache.compose(packet, sha256)

            if (p) {
              // potentially emit err from readNetworkMessage on more verbose logging
              const { data: message } = readNetworkMessage(p.message, identity.publicKey)
              if (message) peer.onPacket({ ...p, message }, port, address)
            }
          } else {
            // potentially emit err from readNetworkMessage on more verbose logging
            const { data: message } = readNetworkMessage(packet.message, identity.publicKey)
            if (message) peer.onPacket({ ...packet, message }, port, address)
          }
        }
      }

      // relay packet forward, if NOT intended for this device, return it so we can
      // predicate the multicast.
      return !identities[0]?.privateKey
    }
  }
}

export default { PTP }

export function formatPeerId (forPubKey) {
  return toBase64Str(forPubKey ?? new Uint8Array(0))
}

export function generateNetworkIdentity () {
  const { publicKey, privateKey } = sodium.crypto_sign_keypair()
  const encPublicKey = toCurve25519pk(publicKey)
  const encPrivateKey = toCurve25519sk(privateKey)
  return { publicKey, privateKey, encPublicKey, encPrivateKey }
}

export function registerNetworkIdentity (keyInfo) {
  if (!keyInfo?.publicKey) {
    return { err: new Error('Missing public-key') }
  }

  const pubKeyStr = toBase64Str(keyInfo.publicKey)
  const pubKeyBuf = toUint8Array(keyInfo.publicKey)
  const privKeyBuf = toUint8Array(keyInfo.privateKey ?? '')

  const encPubKeyBuf = toUint8Array(
    keyInfo.encPublicKey ??

    // need to re-derive the Curve25519 public-key from
    // the provided Ed25519 public-key?
    toCurve25519pk(pubKeyBuf)
  )

  if (encPubKeyBuf.byteLength !== 32) {
    return { err: new Error('Invalid encryption public-key') }
  }

  const encPrivKeyBuf = toUint8Array(
    keyInfo.encPrivateKey ??

    // need to re-derive the Curve25519 private-key from
    // the provided Ed25519 private-key?
    toCurve25519sk(privKeyBuf)
  )

  const isGroupIdentity = keyInfo.members?.length > 0
  const isLocalIdentity = privKeyBuf.byteLength > 0
  const rememberDefaultIdentity = !isGroupIdentity && isLocalIdentity && !defaultIdentity

  if (isGroupIdentity && keyInfo.members.length > MAX_GROUP_MEMBERS) {
    return { err: new Error(`Exceeded max group members of ${MAX_GROUP_MEMBERS}`) }
  }

  const defaultIdentityLabel = (
    isGroupIdentity && isLocalIdentity
      ? 'peer group (this device)'
      : isGroupIdentity
        ? 'peer group'
        : rememberDefaultIdentity
          ? 'this device (default)'
          : isLocalIdentity
            ? 'this device'
            : 'peer device'
  )

  const networkIdentity = networkIdentities[pubKeyStr] = (
    pubKeyStr in networkIdentities
      ? networkIdentities[pubKeyStr]
      // new network-identity entry
      : {
          label: defaultIdentityLabel
        }
  )
  // preserve private-keys (if any) for local identity
  const privateKeysIfAny = (
    isLocalIdentity
      ? {
          privateKey: privKeyBuf,
          encPrivateKey: encPrivKeyBuf
        }

      : null
  )
  // set or update public-keys and private-keys (if any)
  Object.assign(
    networkIdentity,
    {
      publicKey: pubKeyBuf,
      encPublicKey: encPubKeyBuf
    },
    privateKeysIfAny
  )

  // overriding previous label?
  networkIdentity.label = keyInfo.label ?? networkIdentity.label

  // registering a group identity?
  if (isGroupIdentity) {
    networkIdentity.members =
      new Set([
        ...(networkIdentity.members ?? []),
        ...(keyInfo.members.map(toBase64Str) ?? [])
      ])

    for (const memberPubKeyStr of networkIdentity.members) {
      // register this member network-identity (if not already)
      // note: will not return error, so ignoring return value
      registerNetworkIdentity({ publicKey: memberPubKeyStr })

      // link member identity to group identity
      networkIdentities[memberPubKeyStr].groups.add(networkIdentity)
    }

    return { data: true }
  }

  // otherwise, if we get here, registering a single
  // device identity
  networkIdentity.groups = networkIdentity.groups ?? new Set()

  // remember default local identity?
  if (rememberDefaultIdentity) {
    defaultIdentity = networkIdentity
  }

  return { data: true }
}

export function lookupNetworkIdentities (forPubKey = defaultIdentity?.publicKey) {
  const forPubKeyStr = toBase64Str(forPubKey)
  const identities = []
  const keysLookedUp = new Set()
  let keysToLookup = forPubKey ? [forPubKeyStr] : []

  while (keysToLookup.length > 0) {
    const pubKeyStr = keysToLookup.shift()
    keysLookedUp.add(pubKeyStr)
    if (pubKeyStr in networkIdentities) {
      // private keys (if any) to return, for local identity
      const privateKeysIfAny = (
        'privateKey' in networkIdentities[pubKeyStr]
          ? {
              privateKey: toBase64Str(networkIdentities[pubKeyStr].privateKey),
              encPrivateKey: toBase64Str(networkIdentities[pubKeyStr].encPrivateKey)
            }

          : null
      )

      // include network-identity in result
      identities.push({
        label: networkIdentities[pubKeyStr].label,

        publicKey: toBase64Str(networkIdentities[pubKeyStr].publicKey),
        encPublicKey: toBase64Str(networkIdentities[pubKeyStr].encPublicKey),

        ...privateKeysIfAny
      })

      // network-identity part of any groups?
      if (networkIdentities[pubKeyStr].groups?.size > 0) {
        const moreKeysToLookup =
          [...networkIdentities[pubKeyStr].groups]
            .map(identity => toBase64Str(identity.publicKey))
            .filter(pubKeyStr => !keysLookedUp.has(pubKeyStr))

        if (moreKeysToLookup.length > 0) {
          keysToLookup = [...keysToLookup, ...moreKeysToLookup]
        }
      }
    }
  }

  return identities
}

function prepareNetworkMessage (message, forPubKey) {
  const forPubKeyStr = toBase64Str(forPubKey)

  // intended recipient's public-key (encryption)
  // registered?
  if (!networkIdentities[forPubKeyStr]?.encPublicKey) {
    throw new Error('Unable to prepare network message (unknown key)')
  }

  const networkIdentity = networkIdentities[forPubKeyStr]
  try {
    return sodium.crypto_box_seal(message, networkIdentity.encPublicKey)
  } catch (e) {
    throw new Error('Unable to prepare network message', { cause: e })
  }
}

export function readNetworkMessage (message, forPubKey = defaultIdentity?.publicKey) {
  const forPubKeyStr = toBase64Str(forPubKey)

  // intended identity's private-key (decryption) registered?
  if (!networkIdentities[forPubKeyStr]?.encPrivateKey) {
    return { err: new Error('Unable to read network message (unknown key)') }
  }

  const networkIdentity = networkIdentities[forPubKeyStr]

  try {
    const data = sodium.crypto_box_seal_open(message, networkIdentity.encPublicKey, networkIdentity.encPrivateKey)
    return { data }
  } catch (e) {
    return {
      err: new Error('Unable to read network message', { cause: e })
    }
  }
}

export function buildIdentityGroupMessage (groupPubKey, label = 'peer group') {
  const groupPubKeyStr = toBase64Str(groupPubKey)
  const groupIdentity = networkIdentities[groupPubKeyStr]

  // group identity not found or can't be used?
  if (!groupIdentity?.privateKey || !(groupIdentity?.members?.size > 0)) {
    return {
      err: new Error('Unable to build identity-group message (unknown/empty group)')
    }
  }

  // alphabetically-sorted list of group-members (public keys)
  const groupMemberKeys = [...groupIdentity.members].sort()

  let groupSignatureStr
  try {
    // sign group-message
    groupSignatureStr = toBase64Str(sodium.crypto_sign_detached(
      // group member keys concatenated, separated by ';'
      groupMemberKeys.join(';'),
      groupIdentity.privateKey
    ))
  } catch (e) {
    return {
      err: new Error('Unable to generate group key signature', { cause: e })
    }
  }

  let memberSignatures
  try {
    // generate signatures for each local identity
    memberSignatures = groupMemberKeys
      .map(memberPubKey => networkIdentities[memberPubKey])
      .filter(localIdentity => !!localIdentity?.privateKey)
      .reduce(
        (sigs, localIdentity) => ({
          ...sigs,

          // local identity signature
          [toBase64Str(localIdentity.publicKey)]:
            toBase64Str(sodium.crypto_sign_detached(
              // prove authorization to be included in
              // this specific group
              groupPubKeyStr,
              localIdentity.privateKey
            ))
        }),
        {}
      )
  } catch (e) {
    return {
      err: new Error('Unable to generate identity signatures', { cause: e })
    }
  }

  // no valid signatures?
  if (Object.keys(memberSignatures).length === 0) {
    return {
      err: new Error('Unable to build identity-group message (invalid group)')
    }
  }

  // identity-group message
  const msg = {
    p: groupPubKeyStr,
    s: groupSignatureStr,
    ...memberSignatures
  }

  // include label in message?
  if (label) msg.l = label

  // double-check length of JSON serialization
  // (to fit in single packet)
  const json = JSON.stringify(msg)
  if (json.length > 1024) {
    // group label too long?
    if (msg.l.length > MAX_GROUP_LABEL_LENGTH) {
      return {
        err: new Error('Unable to build identity-group message (label too long)')
      }
    }

    return {
      err: new Error('Unable to build identity-group message (message too long)')
    }
  }

  // assemble group-identity message (JSON)
  return {
    data: msg
  }
}

export function verifyIdentityGroupMessage (groupMsg) {
  let groupPubKeyBuf, groupPubKeyStr, groupSignatureBuf, groupMembers
  try {
    ({
      groupPublicKey: {
        buf: groupPubKeyBuf,
        str: groupPubKeyStr
      },
      groupSignature: {
        buf: groupSignatureBuf
      },
      groupMembers
    } = decodeIdentityGroupMessage(groupMsg))
  } catch (e) {
    return {
      err: new Error('Unable to decode identity-group message', { cause: e })
    }
  }

  // invalid (empty) group?
  if (!Array.isArray(groupMembers) || groupMembers.length === 0) {
    return {
      err: new Error('Invalid identity-group message (no members)')
    }
  }

  const memberPubKeyStrs = []

  // process group members
  for (const groupMember of groupMembers) {
    // destructure group member data
    let memberPubKeyBuf, memberPubKeyStr, memberSignatureBuf
    try {
      ({
        publicKey: {
          buf: memberPubKeyBuf,
          str: memberPubKeyStr
        },
        signature: {
          buf: memberSignatureBuf
        }
      } = groupMember)
    } catch (e) {
      return {
        err: new Error('Invalid identity-group member', { cause: e })
      }
    }

    memberPubKeyStrs.push(memberPubKeyStr)

    let isValid
    try {
      // validate each member signature
      isValid = sodium.crypto_sign_verify_detached(memberSignatureBuf, groupPubKeyStr, memberPubKeyBuf)
    } catch (e) {
      return { data: false }
    }

    if (!isValid) return { data: false }
  }

  const memberPubKeys = memberPubKeyStrs.join(';')
  try {
    // validate overall group signature
    return {
      data: sodium.crypto_sign_verify_detached(groupSignatureBuf, memberPubKeys, groupPubKeyBuf)
    }
  } catch (e) {
    return { data: false }
  }
}

export function registerRemoteIdentityGroup (groupMsg) {
  const { data, err } = verifyIdentityGroupMessage(groupMsg)
  if (err || !data) {
    return {
      err: new Error('Unable to verify remote identity group', { cause: err })
    }
  }

  // note: since we already verified the message,
  // decoding should never throw; skipping try..catch
  const {
    groupLabel,
    groupPublicKey: {
      buf: groupPubKeyBuf
    },
    groupMembers
  } = decodeIdentityGroupMessage(groupMsg)

  let res
  try {
    res = registerNetworkIdentity({
      label: groupLabel,
      publicKey: groupPubKeyBuf,
      members: groupMembers.map(member => member.publicKey.str)
    })
  } catch (e) {
    return {
      err: new Error('Unable to register remote identity group', { cause: e })
    }
  }

  // registration error?
  if (res.err) return res

  return { data: true }
}

// NOTE: this function should be used with CAUTION, as
// clearing the registry may permanently discard
// network identity credentials, and thus jeopardize
// user data that was encrypted with/for them
export function _clearRegistry (confirmStr) {
  if (confirmStr !== 'Yes I know what I am doing') {
    return { err: new Error('Please confirm.') }
  }

  // clear identities registry
  for (const key of Object.keys(networkIdentities)) {
    delete networkIdentities[key]
  }
  defaultIdentity = null

  // indicates success
  return { data: true }
}

// ****************************

function toBase64Str (val) {
  return (typeof val === 'string'
    ? val
    : sodium.to_base64(val, sodium.base64_variants.ORIGINAL)
  )
}

function toUint8Array (val) {
  return (typeof val !== 'string'
    ? val
    : sodium.from_base64(val, sodium.base64_variants.ORIGINAL)
  )
}

function toCurve25519pk (ed25519pk) {
  try {
    return sodium.crypto_sign_ed25519_pk_to_curve25519(ed25519pk)
  } catch (e) {}
  return new Uint8Array(0)
}

function toCurve25519sk (ed25519sk) {
  try {
    return sodium.crypto_sign_ed25519_sk_to_curve25519(ed25519sk)
  } catch (e) {}
  return new Uint8Array(0)
}

function decodeIdentityGroupMessage (groupMsg = {}) {
  const {
    l: groupLabel,
    p: groupPubKeyStr,
    s: groupSignatureStr,
    ...memberSignatures
  } = groupMsg
  const groupMembers = []

  // decode members
  for (const [memberPubKeyStr, memberSignatureStr] of Object.entries(memberSignatures)) {
    groupMembers.push({
      publicKey: {
        buf: toUint8Array(memberPubKeyStr),
        str: memberPubKeyStr
      },
      signature: {
        buf: toUint8Array(memberSignatureStr),
        str: memberSignatureStr
      }
    })
  }

  // sort members alphabetically by public-key's base64
  // string representation
  groupMembers.sort((member1, member2) => (
    member1.publicKey.str < member2.publicKey.str
      ? -1
      : member1.publicKey.str > member2.publicKey.str
        ? 1
        : 0
  ))

  return {
    groupLabel,
    groupPublicKey: {
      buf: toUint8Array(groupPubKeyStr),
      str: groupPubKeyStr
    },
    groupSignature: {
      buf: toUint8Array(groupSignatureStr),
      str: groupSignatureStr
    },
    groupMembers
  }
}
