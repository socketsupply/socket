# SYNOPSIS

The "Stream Relay" protocol is a P2P networking protocol.


# DESCRIPTION

This is a [spec](./spec/README.md) and [implementation](./src) for NAT traversal
(hole punching) and highly reliable delivery of datagram packets between peers.


# SCOPE

The protocol is limited to connecting peers and delivering datagrams with a high
degree of reliably. It sits directly on top of UDP and is suitable for building
applications and protocols.

```text
┌───────────┬──────────────────────────────────────────────────────────┐
│ Software  │ Salesforce, Figma, Instagram, Metamask                   │
├───────────┼──────────────────────────────────────────────────────────┤
│ Library   │ GunDB, Hypercore, YJS, IPFS                              │
├───────────┼──────────────────────────────────────────────────────────┤
│ Protocol  │ Stream Relay Protocol                                    │
├───────────┼──────────────────────────────────────────────────────────┤
│ Runtime   │ Node.js, Socket, Deno                                    │
├───────────┼──────────────────────────────────────────────────────────┤
│ OS        │ UDP Socket                                               │
└───────────┴──────────────────────────────────────────────────────────┘
```


# MOTIVATION

## SERVERS OPTIONAL

Servers become more expensive and less reliable as demand scales up. One server
to many users creates a natural bottle-neck, and scaling them up quickly becomes
a complex distribued system of shared state (complexity that far exceeds that of
any p2p program).

When the Cloud was created, there was less hardware in the world than there is
today. However, hardware has become ubiquitous enough that it can be networked
without centralized services. A side effect can be reduced cost, less
complexity, and improved autonomy. The primary motive of this protocol is to
make servers optional.


# KEY CONCEPTS

## Summary

- The protocol provides comprehensive NAT traversal. This is the ability to
connect peers.

- The protocol provides a method for peers to discover each other.

- The protocol is highly partition tolerant. A peer is able to send messages
that can be eventually received by the intended recipient regardless of packet
loss or poor network conditions.

- Packets MAY be delivered in any order and will become eventually consistent;
A `packetId` is `sha256(previousId, message)` which makes packets secure,
content addressable, and verifiable. Causally ordered packets means resumable
streams and uploads.

- Peers cache a minimum of K packets from other peers in the network, each
packet has a configurable TTL and "postage value". When the packet expires it is
rebroadcast with one less postage value, or purged if it has no postage value.

- Encryption and signing is an application-level concern, which makes
remediation of vulnerability disclosures easier.


## NAT traversal

### Problem Description

With client-server architecture, any client can directly request a response from
a server, . However in peer-to-peer architecture, any client can not request a
response from any other client.

P2P software needs to listen for new messages from new people. However most
routers will discard unsolicited packets. It is possible to work around this
problem.

P2P connectivity requires coordination between three or more peers. Consider the
following analogy.

Alice wants to write a letter to Bob. But Bob moves frequently, and she has no
other way to contact him. Before Alice can send Bob the letter, she needs to
learn his address. The solution is for her to ask some friends who might have
talked to Bob recently.

In this anology, Alice's letter is really a packet of data. Bob's address is his
external `IP address` and `port`. And their friends, are a serializable list of
recently known IP addresses and ports. You can read more about the technical
details in the `STATE_0` section of the [spec][3].



## Fixing the Small Network Problem

### Problem Description

Imagine a scenario where Alice and Bob are sending each other messages. Alice
goes offline. Bob continues to write messages to Alice. And because the App Bob
is using has a good "local first" user experience, it appears to Bob as if
everything is fine and that Alice should eventually receive all his messages,
so he also goes offline. When Alice comes online, she doesn’t see Bob’s most
recent messages because he’s not online to share them.

### Problem Summary & Solution Deficits

This is a very common problem with P2P, it’s sometimes called the Small Network
Problem. How well data can survive in a network with this problem is referred to
as "partition tolerance". This problem is often solved by using
[STUN and TURN][1] (relay servers), but these add additional infrastructure that
comes at a cost (both in terms of time, money, and expertise).

### Solution

The way Stream Relay Protocol solves this problem is with a shared, global
packet caching strategy. Every peer in the entire network allocates a small
budget (16Mb by default) for caching packets from any other peer in the network.

A peer will cache random packets with a preference for packets that have a topic
ID that the peer subscribes to. A packet starts out with a postage value of 16
(A peer will never accept a packet with a postage value greater than 16). When a
packet nears its max-age, it is re-broadcast to 3 more random peers in the
network, each taxing its postage by a value of 1 when received, unless its
postage value is 0, in which case it is no longer re-broadcast and is purged
from the peer’s cache.

When a message is published, it is also re-broadcast by at least 3 random peers,
with a preference for the intended recipient and peers that subscribe to the
same topic. The result of this is a high `r-value` (or `r0`, also known as Basic
Reproduction Ratio in epidemiology).

### Solution Summary & Solution Gains

In [simulations][2], a network of `128 peers`, where the join and drop rate are
`>95%`, `+/-30%` of NATs are hard, and only `+/-50%` of the peers subscribe to
the topic ID; an unsolicited packet can replicate to `100%` of the subscribers
in an average of `2.5 seconds`, degrading to only `50%` after a 1000% churn
(unrealistically hostile network conditions) over a `>1 minute` period.

This strategy also improves as the number of participants in the network
increases, since the size of the cache and time packets need to live in it is
reduced. If we revisit our original problem with this strategy, we can
demonstrate a high degree of partition tolerance. It can be said that the peers
in a network act as relays (hence the name of the protocol).

Bob continues to write (optionally encrypted) messages to Alice after she goes
offline, and his packets are published to the network. A network of only 1024
peers (split across multiple apps), will provide Bob’s packets a 100% survival
rate over a 24 hour period, without burdening any particular peer with storage
or compute requirements. Allowing Alice to return to the network and see Bob’s
most recent messages without the need for additional infrastructure.


## Reliable packet delivery

TCP is oftern thought of as an ideal choice for packet delivery since it's
considered "reliable". With TCP packet loss, all packets are withheld until all
packets are received, this can be a delay of up to 1s (as per RFC6298 section
2.4). If the packet can't be retransmitted, an exponential backoff could lead to
another 600ms of delay needed for retransmission.

In fact, `Head-of-Line Blocking` is generally a problem with any ordered stream,
TCP (or UDP with additional higher level protocol code for solving this problem).

TCP introduces other unwanted complexity that makes it less ideal for P2P.

UDP is only considered "unreliable" in the way that packets are not guaranteed
to be delivered. However, UDP is ideal for P2P networks because it’s message
oriented and connectionless (ideal for NAT traversal). Also because of its
message oriented nature, it’s light weight in terms of resource allocation. It's
the responsibility of a higher level protocol to implement a strategy for
ensuring UDP packets are delivered.

Stream Relay Protocol eliminates Head-of-Line blocking entirely by reframing
packets as content-addressable Doubly-Linked lists, allowing packets to be
delivered out of order and become eventually consistent. Causal ordering is made
possible by traversing the previous ID or next ID to determine if there were
packets that came before or after one that is known.

And in the case where there is loss (or simply missing data), the receiver MAY
decide to request the packet. If the peer becomes unavailable, query the network
for the missing packet.

The trade-off is more data is required to re-frame the packet. The average
[MTU][W2] for a UDP packet is ~1500 bytes. Stream Relay Protocol uses ~134 bytes
for framing, allocating `1024` bytes of application or protocol data, which is
[more than enough][E0].

```ascii
┌─────────┬───────────┬──────────┬──────┬───────┬─────────┬─────────┐
│ PREV ID │ PACKET ID │ TOPIC ID │ TYPE │ INDEX │ POSTAGE │ NEXT ID │
│ 32b     │ 32b       │ 32b      │ 1b   │ 4b    │ 1b      │ 32b     │
├─────────┴───────────┴──────────┴──────┴───────┴─────────┴─────────┤
│ MESSAGE BODY                                                      │
│ 1024b                                                             │
└───────────────────────────────────────────────────────────────────┘
```

A packet has a `Next ID` when a message with a length > 1024 bytes is published.
The message body is split into packets that contain max 1024 bytes each. Each
packet is assigned an index. And as usual the packet `ID` is `hash(prev, msg)`,
the `Next ID` is the hash of the next msg and `Previous ID`.

```ascii
                      ┌────────────────────┐     ┌┈ ┈
                ┌─────┼───────┐            │     │
┌──────┬─────┬──┴─┬───┴──┐ ┌──┴───┬─────┬──┴─┬───┴──┐
│ PREV │ ... │ ID │ NEXT │ │ PREV │ ... │ ID │ NEXT │
├──────┴─────┴────┴──────┤ ├──────┴─────┴────┴──────┤ ...
│ MESSAGE                │ │ MESSAGE                │
└────────────────────────┘ └────────────────────────┘
```


# SECURITY

In what ways can a network of shared responsibility be attacked...

- Rate limiting by address addresses flooding.

- The fact that peers in a p2p network are unreliable, and online infrequently
makes DDoS attacks ineffective (what makes DDoS attacks effective is a highly
available target in a many-to-one configuration.

- Verifying packet properties and escaping data


# FURTHER READING

- See the [specification](./spec/README.md)
- See the [source code](./src)


[W0]:https://en.wikipedia.org/wiki/Doubly_linked_list
[W1]:https://en.wikipedia.org/wiki/Universally_unique_identifier
[W2]:https://en.wikipedia.org/wiki/Maximum_transmission_unit
[E0]:https://gafferongames.com/post/snapshot_compression/
[1]:https://datatracker.ietf.org/doc/html/rfc5766
[2]:/test/cases/ratio.js
[3]:/spec/README.md#state_0-initial

[R0]:https://lamport.azurewebsites.net/tla/proving-safety.pdf
[R1]:https://lamport.azurewebsites.net/pubs/liveness.pdf
[R2]:https://pdos.csail.mit.edu/papers/p2pnat.pdf
[R3]:https://www.microsoft.com/en-us/research/uploads/prod/2018/05/book-02-08-08.pdf

[W0]:https://en.wikipedia.org/wiki/UDP_hole_punching
[W1]:https://en.wikipedia.org/wiki/Transport_layer
[W2]:https://en.wikipedia.org/wiki/Rendezvous_protocol
[W3]:https://en.wikipedia.org/wiki/STUN

[T0]:https://tailscale.com/blog/how-nat-traversal-works
[F0]:https://fossbytes.com/connection-oriented-vs-connection-less-connection/

[B1]:https://www.bittorrent.org/beps/bep_0055.html
[C0]:https://github.com/clostra/libutp
[GH01]:https://github.com/libpcp/pcp
[GH02]:https://github.com/paullouisageneau/libplum

[rfc3022]:https://datatracker.ietf.org/doc/html/rfc3022
[rfc2663]:https://datatracker.ietf.org/doc/html/rfc2663
[rfc6886]:https://datatracker.ietf.org/doc/html/rfc6886
[rfc6887]:https://datatracker.ietf.org/doc/html/rfc6887
[rfc791]:https://datatracker.ietf.org/doc/html/rfc791
[rfc1122]:https://datatracker.ietf.org/doc/html/rfc1122
[rfc2460]:https://datatracker.ietf.org/doc/html/rfc2460
