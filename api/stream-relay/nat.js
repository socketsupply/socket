/**
 * The NAT type is encoded using 5 bits:
 *
 * 0b00001 : the lsb indicates if endpoint dependence information is included
 * 0b00010 : the second bit indicates the endpoint dependence value
 *
 * 0b00100 : the third bit indicates if firewall information is included
 * 0b01000 : the fourth bit describes which requests can pass the firewall, only known IPs (0) or any IP (1)
 * 0b10000 : the fifth bit describes which requests can pass the firewall, only known ports (0) or any port (1)
 */

/**
 * Every remote will see the same IP:PORT mapping for this peer.
 *
 *                        :3333 ┌──────┐
 *   :1111                ┌───▶ │  R1  │
 * ┌──────┐    ┌───────┐  │     └──────┘
 * │  P1  ├───▶│  NAT  ├──┤
 * └──────┘    └───────┘  │     ┌──────┐
 *                        └───▶ │  R2  │
 *                        :3333 └──────┘
 */
export const MAPPING_ENDPOINT_INDEPENDENT = 0b00011

/**
 * Every remote will see a different IP:PORT mapping for this peer.
 *
 *                        :4444 ┌──────┐
 *   :1111                ┌───▶ │  R1  │
 * ┌──────┐    ┌───────┐  │     └──────┘
 * │  P1  ├───▶│  NAT  ├──┤
 * └──────┘    └───────┘  │     ┌──────┐
 *                        └───▶ │  R2  │
 *                        :5555 └──────┘
 */
export const MAPPING_ENDPOINT_DEPENDENT = 0b00001

/**
 * The firewall allows the port mapping to be accessed by:
 * - Any IP:PORT combination (FIREWALL_ALLOW_ANY)
 * - Any PORT on a previously connected IP (FIREWALL_ALLOW_KNOWN_IP)
 * - Only from previously connected IP:PORT combinations (FIREWALL_ALLOW_KNOWN_IP_AND_PORT)
 */
export const FIREWALL_ALLOW_ANY = 0b11100
export const FIREWALL_ALLOW_KNOWN_IP = 0b01100
export const FIREWALL_ALLOW_KNOWN_IP_AND_PORT = 0b00100

/**
 * The initial state of the nat is unknown and its value is 0
 */
export const UNKNOWN = 0

/**
 * Full-cone NAT, also known as one-to-one NAT
 *
 * Any external host can send packets to iAddr:iPort by sending packets to eAddr:ePort.
 *
 * @summary its a packet party at this mapping and everyone's invited
 */
export const UNRESTRICTED = (FIREWALL_ALLOW_ANY | MAPPING_ENDPOINT_INDEPENDENT)

/**
 * (Address)-restricted-cone NAT
 *
 * An external host (hAddr:any) can send packets to iAddr:iPort by sending packets to eAddr:ePort only
 * if iAddr:iPort has previously sent a packet to hAddr:any. "Any" means the port number doesn't matter.
 *
 * @summary The NAT will drop your packets unless a peer within its network has previously messaged you from *any* port.
 */
export const ADDR_RESTRICTED = (FIREWALL_ALLOW_KNOWN_IP | MAPPING_ENDPOINT_INDEPENDENT)

/**
 * Port-restricted cone NAT
 *
 * An external host (hAddr:hPort) can send packets to iAddr:iPort by sending
 * packets to eAddr:ePort only if iAddr:iPort has previously sent a packet to
 * hAddr:hPort.
 *
 * @summary The NAT will drop your packets unless a peer within its network
 * has previously messaged you from this *specific* port.
 */
export const PORT_RESTRICTED = (FIREWALL_ALLOW_KNOWN_IP_AND_PORT | MAPPING_ENDPOINT_INDEPENDENT)

/**
 * Symmetric NAT
 *
 * Only an external host that receives a packet from an internal host can send
 * a packet back.
 *
 * @summary The NAT will only accept replies to a correspondence initialized
 * by itself, the mapping it created is only valid for you.
 */
export const ENDPOINT_RESTRICTED = (FIREWALL_ALLOW_KNOWN_IP_AND_PORT | MAPPING_ENDPOINT_DEPENDENT)

/**
 * Returns true iff a valid MAPPING_ENDPOINT_* flag has been added (indicated by the lsb).
 */
export const isEndpointDependenceDefined = nat => (nat & 0b00001) === 0b00001

/**
 * Returns true iff a valid FIREWALL_ALLOW_* flag has been added (indicated by the third bit).
 */
export const isFirewallDefined = nat => (nat & 0b00100) === 0b00100

/**
 * Returns true iff both FIREWALL_ALLOW_* and MAPPING_ENDPOINT_* flags have been added.
 */
export const isValid = nat => isEndpointDependenceDefined(nat) && isFirewallDefined(nat)

/**
 * toString returns a human-readable label for the NAT enum
 */
export const toString = n => {
  switch (n) {
    case UNRESTRICTED: return 'UNRESTRICTED'
    case ADDR_RESTRICTED: return 'ADDR_RESTRICTED'
    case PORT_RESTRICTED: return 'PORT_RESTRICTED'
    case ENDPOINT_RESTRICTED: return 'ENDPOINT_RESTRICTED'
    default: return 'UNKNOWN'
  }
}

/**
 * toStringStrategy returns a human-readable label for the STRATEGY enum
 */
export const toStringStrategy = n => {
  switch (n) {
    case STRATEGY_DEFER: return 'STRATEGY_DEFER'
    case STRATEGY_DIRECT_CONNECT: return 'STRATEGY_DIRECT_CONNECT'
    case STRATEGY_TRAVERSAL_OPEN: return 'STRATEGY_TRAVERSAL_OPEN'
    case STRATEGY_TRAVERSAL_CONNECT: return 'STRATEGY_TRAVERSAL_CONNECT'
    case STRATEGY_PROXY: return 'STRATEGY_PROXY'
    default: return 'STRATEGY_UNKNOWN'
  }
}

export const STRATEGY_DEFER = 0 // do nothing and let the other side initialize the connection
export const STRATEGY_DIRECT_CONNECT = 1 // simply connect
export const STRATEGY_TRAVERSAL_OPEN = 2 // spam random ports and listen for replies
export const STRATEGY_TRAVERSAL_CONNECT = 3 // spam random ports but dont bother listening
export const STRATEGY_PROXY = 4 // use a third-party proxy

/**
 * ConnectionStrategy returns the best strategy to use when attempting to connect from a->b.
 */
export const connectionStrategy = (a, b) => {
  switch (b) {
    // b always accepts connections
    case UNRESTRICTED: return STRATEGY_DIRECT_CONNECT

    // b only accepts connections from an IP it has previously communicated with
    case ADDR_RESTRICTED: {
      switch (a) {
        case UNRESTRICTED: return STRATEGY_DEFER
        case ADDR_RESTRICTED: return STRATEGY_DIRECT_CONNECT // both sides attempt, one will succeed
        case PORT_RESTRICTED: return STRATEGY_DIRECT_CONNECT // a is hinting, b is guessing
        case ENDPOINT_RESTRICTED: return STRATEGY_DIRECT_CONNECT // a is hinting, b is guessing
      }
      break
    }

    // b only accepts connections from an IP:PORT it has previously communicated with
    case PORT_RESTRICTED: {
      switch (a) {
        case UNRESTRICTED: return STRATEGY_DEFER
        case ADDR_RESTRICTED: return STRATEGY_DIRECT_CONNECT // a is guessing, b is hinting
        case PORT_RESTRICTED: return STRATEGY_DIRECT_CONNECT // both guess, will take too long, most resign to proxying
        case ENDPOINT_RESTRICTED: return STRATEGY_TRAVERSAL_CONNECT // try connecting
      }
      break
    }

    // b only accepts replies to its own messages
    case ENDPOINT_RESTRICTED: {
      switch (a) {
        case UNRESTRICTED: return STRATEGY_DEFER
        case ADDR_RESTRICTED: return STRATEGY_DIRECT_CONNECT // the 3 successive packets will penetrate
        case PORT_RESTRICTED: return STRATEGY_TRAVERSAL_OPEN // open up some ports
        case ENDPOINT_RESTRICTED: return STRATEGY_PROXY // unroutable
      }
    }
  }

  return STRATEGY_PROXY
}
