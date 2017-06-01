#include <pgm_socket.h>

#include <boost/assert.hpp>

namespace umdgw {

#ifndef MSG_ERRQUEUE
#define MSG_ERRQUEUE 0x2000
#endif

//  Maximum transport data unit size for PGM (TPDU).
static const int pgm_max_tpdu = 1500;

//  Maximal batching size for engines with receiving functionality.
//  So, if there are 10 messages that fit into the batch size, all of
//  them may be read by a single 'recv' system call, thus avoiding
//  unnecessary network stack traversals.
static const int in_batch_size = 8192;

static int pgm_inet_pton (
	int	af, const char* restrict src, void*	restrict dst) {
  struct addrinfo hints, *result;
  memset (&hints, 0, sizeof(hints));
  hints.ai_family		= af;
  hints.ai_socktype	= SOCK_STREAM;		/* not really */
  hints.ai_protocol	= IPPROTO_TCP;		/* not really */
  hints.ai_flags		= AI_NUMERICHOST;
  {
  const int e = getaddrinfo (src, NULL, &hints, &result);
  if (0 != e) {
    return 0;	/* error */
  }

  BOOST_ASSERT(NULL != result->ai_addr);
  BOOST_ASSERT(0 != result->ai_addrlen);

  switch (result->ai_addr->sa_family) {
  case AF_INET: 
    {
      struct sockaddr_in s4;
      memcpy (&s4, result->ai_addr, sizeof(s4));
      memcpy (dst, &s4.sin_addr.s_addr, sizeof(struct in_addr));
    }
    break;

  case AF_INET6: 
    {
      struct sockaddr_in6 s6;
      memcpy (&s6, result->ai_addr, sizeof(s6));
      memcpy (dst, &s6.sin6_addr, sizeof(struct in6_addr));
    }
    break;

  default:
    BOOST_ASSERT(false);
    break;
  }

  freeaddrinfo (result);
  return 1;	/* success */
  }
}

PGMSocket::PGMSocket(bool receiver, const PGMOptions &options) 
  : sock_(NULL)
  , options_(options)
  , receiver_(receiver)
  , pgm_msgv_(NULL)
  , pgm_msgv_len_(0)
  , nbytes_rec_(0)
  , last_rx_status_(0)
  , last_tx_status_(0)
  , nbytes_processed_(0)
  , pgm_msgv_processed_(0)
  , udp_encapsulation_(false) {
  // nothing
}

PGMSocket::~PGMSocket () {
  if (pgm_msgv_) {
    free(pgm_msgv_);
    pgm_msgv_ = NULL;
  }
  if (sock_) {
    pgm_close(sock_, true);
    sock_ = NULL;
  }
}

//  Resolve PGM socket address.
//  network_ of the form <interface & multicast group decls>:<IP port>
//  e.g. eth0;239.192.0.1:7500
//       link-local;224.250.0.1,224.250.0.2;224.250.0.3:8000
//       ;[fe80::1%en0]:7500
int PGMSocket::InitAddress(const char *address, struct pgm_addrinfo_t **res, 
  uint16_t* port_number) {
  //  Parse port number, start from end for IPv6
  const char *port_delim = strrchr(address, ':');
  if (!port_delim) {
    return VSS_INVALID;
  }

  *port_number = atoi(port_delim + 1);
  
  char network[256];
  if (port_delim - address >= static_cast<int>(sizeof (network)) - 1) {
    return VSS_INVALID;
  }
  memset(network, '\0', sizeof (network));
  memcpy(network, address, port_delim - address);

  pgm_error_t *pgm_error = NULL;
  struct pgm_addrinfo_t hints;

  memset(&hints, 0, sizeof (hints));
  hints.ai_family = AF_UNSPEC;
  if (!pgm_getaddrinfo(network, NULL, res, &pgm_error)) {
    //  Invalid parameters don't set pgm_error_t.
    BOOST_ASSERT(pgm_error != NULL);
    if (pgm_error->domain == PGM_ERROR_DOMAIN_IF &&
      //  NB: cannot catch EAI_BADFLAGS.
      (pgm_error->code != PGM_ERROR_SERVICE &&
       pgm_error->code != PGM_ERROR_SOCKTNOSUPPORT)) {
      //  User, host, or network configuration or transient error.
      pgm_error_free(pgm_error);
      return VSS_INVALID;
    }

    //  Fatal OpenPGM internal error.
    BOOST_ASSERT(false);
  }
  return 0;
}

//  Create, bind and connect PGM socket.
int PGMSocket::Init(bool udp_encapsulation, const char *network) {
  udp_encapsulation_ = udp_encapsulation;
  network_ = network;
  //  Can not open transport before destroying old one.
  BOOST_ASSERT(sock_ == NULL);
  BOOST_ASSERT(options_.rate > 0);

  //  Zero counter used in msgrecv.
  nbytes_rec_ = 0;
  nbytes_processed_ = 0;
  pgm_msgv_processed_ = 0;

  uint16_t port_number;
  struct pgm_addrinfo_t *res = NULL;
  sa_family_t sa_family;

  pgm_error_t *pgm_error = NULL;

  if (InitAddress(network, &res, &port_number) < 0) {
    goto err_abort;
  }

  BOOST_ASSERT(res != NULL);

  //  Pick up detected IP family.
  sa_family = res->ai_send_addrs[0].gsr_group.ss_family;

  //  Create IP/PGM or UDP/PGM socket.
  if (udp_encapsulation) {
    if (!pgm_socket(&sock_, sa_family, SOCK_SEQPACKET, IPPROTO_UDP, 
      &pgm_error)) {
      //  Invalid parameters don't set pgm_error_t.
      BOOST_ASSERT(pgm_error != NULL);
      if (pgm_error->domain == PGM_ERROR_DOMAIN_SOCKET && (
        pgm_error->code != PGM_ERROR_BADF &&
        pgm_error->code != PGM_ERROR_FAULT &&
        pgm_error->code != PGM_ERROR_NOPROTOOPT &&
        pgm_error->code != PGM_ERROR_FAILED)) {
        //  User, host, or network configuration or transient error.
        goto err_abort;
      }
      //  Fatal OpenPGM internal error.
      BOOST_ASSERT (false);
    }

    //  All options are of data type int
    const int encapsulation_port = port_number;
    if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_UDP_ENCAP_UCAST_PORT,
      &encapsulation_port, sizeof (encapsulation_port))) {
      goto err_abort;
    }
        
    if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_UDP_ENCAP_MCAST_PORT,
      &encapsulation_port, sizeof (encapsulation_port))) {
      goto err_abort;
    }
  } else {
    if (!pgm_socket(&sock_, sa_family, SOCK_SEQPACKET, IPPROTO_PGM, 
      &pgm_error)) {
      //  Invalid parameters don't set pgm_error_t.
      BOOST_ASSERT(pgm_error != NULL);
      if (pgm_error->domain == PGM_ERROR_DOMAIN_SOCKET && (
        pgm_error->code != PGM_ERROR_BADF &&
        pgm_error->code != PGM_ERROR_FAULT &&
        pgm_error->code != PGM_ERROR_NOPROTOOPT &&
        pgm_error->code != PGM_ERROR_FAILED)) {
        //  User, host, or network configuration or transient error.
        goto err_abort;
      }
      //  Fatal OpenPGM internal error.
      BOOST_ASSERT (false);
    }
  }

  { // set options
	const int rcvbuf = (int) options_.rcvbuf;
	if (rcvbuf >= 0) {
    if (!pgm_setsockopt(sock_, SOL_SOCKET, SO_RCVBUF, 
      &rcvbuf, sizeof (rcvbuf))) {
      goto err_abort;
    }
	}

	const int sndbuf = (int) options_.sndbuf;
	if (sndbuf >= 0) {
    if (!pgm_setsockopt(sock_, SOL_SOCKET, SO_SNDBUF, 
      &sndbuf, sizeof (sndbuf))) {
      goto err_abort;
    }
	}

	const int max_tpdu = (int) pgm_max_tpdu;
  if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_MTU, 
    &max_tpdu, sizeof (max_tpdu))) {
    goto err_abort;
  }  
  } // set options

  if (receiver_) {
    const int recv_only        = 1,
              rxw_max_tpdu     = (int) pgm_max_tpdu,
              rxw_sqns         = ComputeSqns(rxw_max_tpdu),
              peer_expiry      = pgm_secs(300),
              spmr_expiry      = pgm_msecs (250),//pgm_msecs(25),
              nak_bo_ivl       = pgm_msecs(50),
              nak_rpt_ivl      = pgm_secs (2),//pgm_msecs(200),
              nak_rdata_ivl    = pgm_secs (2),//pgm_msecs(200),
              nak_data_retries = 50,
              nak_ncf_retries  = 50;

    if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_RECV_ONLY, &recv_only,
          sizeof (recv_only)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_RXW_SQNS, &rxw_sqns,
          sizeof (rxw_sqns)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_PEER_EXPIRY, &peer_expiry,
          sizeof (peer_expiry)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_SPMR_EXPIRY, &spmr_expiry,
          sizeof (spmr_expiry)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_NAK_BO_IVL, &nak_bo_ivl,
          sizeof (nak_bo_ivl)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_NAK_RPT_IVL, &nak_rpt_ivl,
          sizeof (nak_rpt_ivl)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_NAK_RDATA_IVL,
          &nak_rdata_ivl, sizeof (nak_rdata_ivl)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_NAK_DATA_RETRIES,
          &nak_data_retries, sizeof (nak_data_retries)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_NAK_NCF_RETRIES,
          &nak_ncf_retries, sizeof (nak_ncf_retries))) {
      goto err_abort;
    }
  } else {
    const int send_only        = 1,
              max_rte          = (int) ((options_.rate * 1000) / 8),
              txw_max_tpdu     = (int) pgm_max_tpdu,
              txw_sqns         = ComputeSqns(txw_max_tpdu),
              ambient_spm      = pgm_secs (30),
              heartbeat_spm[]  = { pgm_msecs (100),
                                    pgm_msecs (100),
                                    pgm_msecs (100),
                                    pgm_msecs (100),
                                    pgm_msecs (1300),
                                    pgm_secs  (7),
                                    pgm_secs  (16),
                                    pgm_secs  (25),
                                    pgm_secs  (30) };

    if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_SEND_ONLY,
          &send_only, sizeof (send_only)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_ODATA_MAX_RTE,
          &max_rte, sizeof (max_rte)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_TXW_SQNS,
          &txw_sqns, sizeof (txw_sqns)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_AMBIENT_SPM,
          &ambient_spm, sizeof (ambient_spm)) ||
      !pgm_setsockopt(sock_, IPPROTO_PGM, PGM_HEARTBEAT_SPM,
          &heartbeat_spm, sizeof (heartbeat_spm))) {
      goto err_abort;
    }    
  }

  //  PGM transport GSI.
  struct pgm_sockaddr_t addr;

  memset (&addr, 0, sizeof(addr));
  addr.sa_port = port_number;
  addr.sa_addr.sport = DEFAULT_DATA_SOURCE_PORT;

  //  Create random GSI.
  uint32_t buf [2];
  buf [0] = GenerateRandom();
  buf [1] = GenerateRandom();
  if (!pgm_gsi_create_from_data(&addr.sa_addr.gsi, (uint8_t*)buf, 8)) {
    goto err_abort;
  }

  //  Bind a transport to the specified network devices.
  struct pgm_interface_req_t if_req;
  memset (&if_req, 0, sizeof(if_req));
  if_req.ir_interface = res->ai_recv_addrs[0].gsr_interface;
  if_req.ir_scope_id  = 0;
  if (AF_INET6 == sa_family) {
    struct sockaddr_in6 sa6;
    memcpy (&sa6, &res->ai_recv_addrs[0].gsr_group, sizeof (sa6));
    if_req.ir_scope_id = sa6.sin6_scope_id;
  }
  if (!pgm_bind3(sock_, &addr, sizeof (addr), &if_req, sizeof (if_req), 
    &if_req, sizeof (if_req), &pgm_error)) {

    //  Invalid parameters don't set pgm_error_t.
    BOOST_ASSERT (pgm_error != NULL);
    if ((pgm_error->domain == PGM_ERROR_DOMAIN_SOCKET ||
      pgm_error->domain == PGM_ERROR_DOMAIN_IF) && (
      pgm_error->code != PGM_ERROR_INVAL &&
      pgm_error->code != PGM_ERROR_BADF &&
      pgm_error->code != PGM_ERROR_FAULT)) {
      //  User, host, or network configuration or transient error.
      goto err_abort;
    }
    //  Fatal OpenPGM internal error.
    BOOST_ASSERT (false);
  }

  //  Join IP multicast groups.
  if (!options_.group_source.empty()) {
    for (unsigned i = 0; i < res->ai_recv_addrs_len; i++) {
      struct sockaddr* addr = 
        (struct sockaddr*)&res->ai_recv_addrs[i].gsr_source;
      pgm_inet_pton (AF_INET, options_.group_source.c_str(), 
        &((struct sockaddr_in*)addr)->sin_addr);
      if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_JOIN_SOURCE_GROUP,
        &res->ai_recv_addrs[i], sizeof (struct group_source_req))) {
        goto err_abort;
      }
    }
  } else {
    for (unsigned i = 0; i < res->ai_recv_addrs_len; i++) {
      if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_JOIN_GROUP,
        &res->ai_recv_addrs[i], sizeof (struct group_req))) {
        goto err_abort;
      }
    }
  }
  
  if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_SEND_GROUP,
    &res->ai_send_addrs[0], sizeof (struct group_req))) {
    goto err_abort;
  }

  pgm_freeaddrinfo (res);
  res = NULL;

  { //  Set IP level parameters.
	// Multicast loopback disabled by default
	const int multicast_loop = options_.multicast_loop ? 1 : 0;
  if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_MULTICAST_LOOP,
    &multicast_loop, sizeof (multicast_loop))) {
    goto err_abort;
  }

	const int multicast_hops = options_.multicast_hops;
  if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_MULTICAST_HOPS,
    &multicast_hops, sizeof (multicast_hops))) {
    goto err_abort;
  }

	//  Expedited Forwarding PHB for network elements, no ECN.
	//  Ignore return value due to varied runtime support.
	const int dscp = 0x2e << 2;
  if (AF_INET6 != sa_family) {
    pgm_setsockopt(sock_, IPPROTO_PGM, PGM_TOS, &dscp, sizeof (dscp));
  }
	const int nonblocking = 1;
  if (!pgm_setsockopt(sock_, IPPROTO_PGM, PGM_NOBLOCK, &nonblocking, 
    sizeof (nonblocking))) {
    goto err_abort;
  }
  } //  Set IP level parameters.

  //  Connect PGM transport to start state machine.
  if (!pgm_connect(sock_, &pgm_error)) {
    //  Invalid parameters don't set pgm_error_t.
    BOOST_ASSERT(pgm_error != NULL);
    goto err_abort;
  }

  //  For receiver transport preallocate pgm_msgv array.
  if (receiver_) {
    BOOST_ASSERT(in_batch_size > 0);
    size_t max_tsdu_size = GetMaxTsduSize();
    pgm_msgv_len_ = (int) in_batch_size / max_tsdu_size;
    if ((int)in_batch_size % max_tsdu_size) {
      pgm_msgv_len_++;
    }
    pgm_msgv_len_ = 1;
    BOOST_ASSERT(pgm_msgv_len_);

    pgm_msgv_ = (pgm_msgv_t*) malloc (sizeof (pgm_msgv_t) * pgm_msgv_len_);
    BOOST_ASSERT(pgm_msgv_);
  }

  return 0;

err_abort:
  if (sock_ != NULL) {
    pgm_close(sock_, FALSE);
    sock_ = NULL;
  }
  if (res != NULL) {
    pgm_freeaddrinfo(res);
    res = NULL;
  }
  if (pgm_error != NULL) {
    pgm_error_free(pgm_error);
    pgm_error = NULL;
  }
  return VSS_INVALID;
}

int PGMSocket::Reset() {
  if (pgm_msgv_) {
    free(pgm_msgv_);
    pgm_msgv_ = NULL;
  }
  last_rx_status_ = 0;
  last_tx_status_ = 0;
  pgm_msgv_len_ = 0;
  nbytes_rec_ = 0;
  nbytes_processed_ = 0;
  pgm_msgv_processed_ = 0;
  if (sock_) {
    pgm_close(sock_, true);
    sock_ = NULL;
  }

  bool udp_encapsulation = udp_encapsulation_;
  std::string network = network_;
  return Init(udp_encapsulation, network.c_str());
}

//  Get receiver fds. receive_fd_ is signaled for incoming packets,
//  waiting_pipe_fd_ is signaled for state driven events and data.
void PGMSocket::GetReceiverFds(fd_t *receive_fd, 
  fd_t *waiting_pipe_fd) {
  socklen_t socklen;
  bool rc;

  BOOST_ASSERT (receive_fd);
  BOOST_ASSERT (waiting_pipe_fd);

  socklen = sizeof (*receive_fd);
  rc = pgm_getsockopt(sock_, IPPROTO_PGM, PGM_RECV_SOCK, receive_fd,
      &socklen);
  BOOST_ASSERT(rc);
  BOOST_ASSERT(socklen == sizeof (*receive_fd));

  socklen = sizeof (*waiting_pipe_fd);
  rc = pgm_getsockopt(sock_, IPPROTO_PGM, PGM_PENDING_SOCK, waiting_pipe_fd,
      &socklen);
  BOOST_ASSERT(rc);
  BOOST_ASSERT(socklen == sizeof (*waiting_pipe_fd));
}

//  Get fds and store them into user allocated memory. 
//  send_fd is for non-blocking send wire notifications.
//  receive_fd_ is for incoming back-channel protocol packets.
//  rdata_notify_fd_ is raised for waiting repair transmissions.
//  pending_notify_fd_ is for state driven events.
void PGMSocket::GetSenderFds(fd_t *send_fd_, fd_t *receive_fd_, 
  fd_t *rdata_notify_fd_, fd_t *pending_notify_fd_) {
  socklen_t socklen;
  bool rc;

  BOOST_ASSERT (send_fd_);
  BOOST_ASSERT (receive_fd_);
  BOOST_ASSERT (rdata_notify_fd_);
  BOOST_ASSERT (pending_notify_fd_);

  socklen = sizeof (*send_fd_);
  rc = pgm_getsockopt(sock_, IPPROTO_PGM, PGM_SEND_SOCK, send_fd_, &socklen);
  BOOST_ASSERT(rc);
  BOOST_ASSERT(socklen == sizeof (*receive_fd_));

  socklen = sizeof (*receive_fd_);
  rc = pgm_getsockopt(sock_, IPPROTO_PGM, PGM_RECV_SOCK, receive_fd_,
      &socklen);
  BOOST_ASSERT(rc);
  BOOST_ASSERT(socklen == sizeof (*receive_fd_));

  socklen = sizeof (*rdata_notify_fd_);
  rc = pgm_getsockopt(sock_, IPPROTO_PGM, PGM_REPAIR_SOCK, rdata_notify_fd_,
      &socklen);
  BOOST_ASSERT(rc);
  BOOST_ASSERT(socklen == sizeof (*rdata_notify_fd_));

  socklen = sizeof (*pending_notify_fd_);
  rc = pgm_getsockopt(sock_, IPPROTO_PGM, PGM_PENDING_SOCK,
      pending_notify_fd_, &socklen);
  BOOST_ASSERT(rc);
  BOOST_ASSERT(socklen == sizeof (*pending_notify_fd_));
}

size_t PGMSocket::GetMaxTsduSize() {
  int max_tsdu = 0;
  socklen_t optlen = sizeof (max_tsdu);

  bool rc = pgm_getsockopt(sock_, IPPROTO_PGM, PGM_MSS, &max_tsdu, &optlen);
  BOOST_ASSERT(rc);
  BOOST_ASSERT(optlen == sizeof (max_tsdu));
  return (size_t) max_tsdu;
}

int PGMSocket::ComputeSqns(int tpdu) {
  //  Convert rate into B/ms.
  uint64_t rate = uint64_t(options_.rate) / 8;
  //  Compute the size of the buffer in bytes.
  uint64_t size = uint64_t(options_.recovery_ivl) * rate;
  //  Translate the size into number of packets.
  uint64_t sqns = size / tpdu;

  //  Buffer should be able to hold at least one packet.
  if (sqns == 0) {
    sqns = 1;
  }
  return (int) sqns;
}

//  Send one APDU, transmit window owned memory.
//  data_len_ must be less than one TPDU.
int PGMSocket::Send(unsigned char *data, size_t data_len, 
  int* o_sent_bytes) {
  int ret = 0;
  size_t nbytes = 0;
  const int status = pgm_send(sock_, data, data_len, &nbytes);

  //  We have to write all data as one packet.
  if (nbytes > 0) {
    BOOST_ASSERT(status == PGM_IO_STATUS_NORMAL);
    BOOST_ASSERT(nbytes == data_len);
  } else {
    BOOST_ASSERT(status == PGM_IO_STATUS_RATE_LIMITED ||
      status == PGM_IO_STATUS_WOULD_BLOCK);

    if (status == PGM_IO_STATUS_RATE_LIMITED) {
      ret = VSS_NOMEM;
    } else {
      ret = VSS_BUSY;
    }
  }

  //  Save return value.
  last_tx_status_ = status;
  *o_sent_bytes = static_cast<int>(nbytes);
  return ret;
}

long PGMSocket::GetRxTimeout() {
  if (last_rx_status_ != PGM_IO_STATUS_RATE_LIMITED &&
    last_rx_status_ != PGM_IO_STATUS_TIMER_PENDING) {
    return -1;
  }

  struct timeval tv;
  socklen_t optlen = sizeof (tv);
  const bool rc = pgm_getsockopt(sock_, IPPROTO_PGM,
    last_rx_status_ == PGM_IO_STATUS_RATE_LIMITED ? PGM_RATE_REMAIN :
    PGM_TIME_REMAIN, &tv, &optlen);
  BOOST_ASSERT(rc);

  const long timeout = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
  return timeout;
}

long PGMSocket::GetTxTimeout() {
  if (last_tx_status_ != PGM_IO_STATUS_RATE_LIMITED) {
    return -1;
  }

  struct timeval tv;
  socklen_t optlen = sizeof (tv);
  const bool rc = pgm_getsockopt(sock_, IPPROTO_PGM, PGM_RATE_REMAIN, &tv,
    &optlen);
  BOOST_ASSERT(rc);

  const long timeout = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
  return timeout;
}

//  pgm_recvmsgv is called to fill the pgm_msgv array up to  pgm_msgv_len.
//  In subsequent calls data from pgm_msgv structure are returned.
int PGMSocket::Receive(void **raw_data, const pgm_tsi_t **tsi, 
  int* o_received_bytes) {
  size_t raw_data_len = 0;

  //  We just sent all data from pgm_transport_recvmsgv up 
  //  and have to return 0 that another engine in this thread is scheduled.
  if (nbytes_rec_ == nbytes_processed_ && nbytes_rec_ > 0) {
    //  Reset all the counters.
    nbytes_rec_ = 0;
    nbytes_processed_ = 0;
    pgm_msgv_processed_ = 0;
    *o_received_bytes = 0;
    return VSS_TIMEOUT;
  }

  //  If we have are going first time or if we have processed all pgm_msgv_t
  //  structure previously read from the pgm socket.
  if (nbytes_rec_ == nbytes_processed_) {

    //  Check program flow.
    BOOST_ASSERT(pgm_msgv_processed_ == 0);
    BOOST_ASSERT(nbytes_processed_ == 0);
    BOOST_ASSERT(nbytes_rec_ == 0);

    //  Receive a vector of Application Protocol Domain Unit's (APDUs) 
    //  from the transport.
    pgm_error_t *pgm_error = NULL;
    const int status = pgm_recvmsgv(sock_, pgm_msgv_,
      pgm_msgv_len_, MSG_ERRQUEUE, &nbytes_rec_, &pgm_error);

    //  Invalid parameters.
    BOOST_ASSERT(status != PGM_IO_STATUS_ERROR);

    last_rx_status_ = status;

    //  In a case when no ODATA/RDATA fired POLLIN event (SPM...)
    //  pgm_recvmsg returns PGM_IO_STATUS_TIMER_PENDING.
    if (status == PGM_IO_STATUS_TIMER_PENDING) {
      BOOST_ASSERT(nbytes_rec_ == 0);
      //  In case if no RDATA/ODATA caused POLLIN 0 is 
      //  returned.
      nbytes_rec_ = 0;
      *o_received_bytes = 0;
      return VSS_BUSY;
    }

    //  Send SPMR, NAK, ACK is rate limited.
    if (status == PGM_IO_STATUS_RATE_LIMITED) {
      BOOST_ASSERT(nbytes_rec_ == 0);

      //  In case if no RDATA/ODATA caused POLLIN 0 is returned.
      nbytes_rec_ = 0;
      *o_received_bytes = 0;
      return VSS_NOMEM;
    }

    //  No peers and hence no incoming packets.
    if (status == PGM_IO_STATUS_WOULD_BLOCK) {

      BOOST_ASSERT(nbytes_rec_ == 0);

      //  In case if no RDATA/ODATA caused POLLIN 0 is returned.
      nbytes_rec_ = 0;
      *o_received_bytes = 0;
      return VSS_TIMEOUT;
    }

    //  Data loss.
    if (status == PGM_IO_STATUS_RESET) {

      struct pgm_sk_buff_t* skb = pgm_msgv_[0].msgv_skb[0];

      //  Save lost data TSI.
      *tsi = &skb->tsi;
      nbytes_rec_ = 0;
      pgm_free_skb (skb);
      *o_received_bytes = -1;
      return VSS_DATA_LOST;
    }

    BOOST_ASSERT (status == PGM_IO_STATUS_NORMAL);
  } else {
    BOOST_ASSERT(pgm_msgv_processed_ <= pgm_msgv_len_);
  }

  // Zero byte payloads are valid in PGM, but not 0MQ protocol.
  BOOST_ASSERT(nbytes_rec_ > 0);

  // Only one APDU per pgm_msgv_t structure is allowed.
  BOOST_ASSERT(pgm_msgv_[pgm_msgv_processed_].msgv_len == 1);
 
  struct pgm_sk_buff_t* skb = 
    pgm_msgv_[pgm_msgv_processed_].msgv_skb [0];

  //  Take pointers from pgm_msgv_t structure.
  *raw_data = skb->data;
  raw_data_len = skb->len;

  //  Save current TSI.
  *tsi = &skb->tsi;

  //  Move the the next pgm_msgv_t structure.
  pgm_msgv_processed_ ++;
  BOOST_ASSERT(pgm_msgv_processed_ <= pgm_msgv_len_);
  nbytes_processed_ += raw_data_len;
  *o_received_bytes = static_cast<int>(raw_data_len);
  return 0;
}

int PGMSocket::ProcessUpstream() {
  pgm_msgv_t dummy_msg;

  size_t dummy_bytes = 0;
  pgm_error_t *pgm_error = NULL;

  const int status = pgm_recvmsgv(sock_, &dummy_msg,
    1, MSG_ERRQUEUE, &dummy_bytes, &pgm_error);

  //  Invalid parameters.
  BOOST_ASSERT(status != PGM_IO_STATUS_ERROR);

  //  No data should be returned.
  BOOST_ASSERT(dummy_bytes == 0 && (status == PGM_IO_STATUS_TIMER_PENDING || 
    status == PGM_IO_STATUS_RATE_LIMITED ||
    status == PGM_IO_STATUS_WOULD_BLOCK));

  last_rx_status_ = status;

  if (status == PGM_IO_STATUS_TIMER_PENDING) {
    return VSS_BUSY;
  } else if (status == PGM_IO_STATUS_RATE_LIMITED) {
    return VSS_NOMEM;
  } else {
    return VSS_TIMEOUT;
  }
}

} // namespace umdgw