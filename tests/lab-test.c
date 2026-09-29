#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "harness/unity.h"
#include "../src/lab.h"
#include "../src/utils.h"

static int mock_io_enabled;
static int mock_send_failure;
static int mock_recv_failure;
static int mock_poll_result = 1;
static int mock_clock_enabled;
static int mock_clock_failure;
static int mock_clock_calls;
static uint8_t mock_receive_packets[4][HEADER_SIZE + PAYLOAD_SIZE];
static size_t mock_receive_sizes[4];
static size_t mock_receive_count;
static size_t mock_receive_index;

static int make_nonblocking_socketpair(int sockets[2])
{
  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sockets) != 0)
  {
    return -1;
  }
  fcntl(sockets[0], F_SETFL, fcntl(sockets[0], F_GETFL, 0) | O_NONBLOCK);
  fcntl(sockets[1], F_SETFL, fcntl(sockets[1], F_GETFL, 0) | O_NONBLOCK);
  return 0;
}

ssize_t test_send(int socket_fd, const void *buffer, size_t length, int flags)
{
  if (mock_io_enabled)
  {
    (void)socket_fd;
    (void)buffer;
    (void)flags;
    if (mock_send_failure)
    {
      errno = EIO;
      return -1;
    }
    return (ssize_t)length;
  }
  return send(socket_fd, buffer, length, flags);
}

ssize_t test_recv(int socket_fd, void *buffer, size_t length, int flags)
{
  if (mock_io_enabled)
  {
    if (mock_recv_failure)
    {
      errno = EIO;
      return -1;
    }
    (void)socket_fd;
    (void)flags;
    if (mock_receive_index >= mock_receive_count)
    {
      return -1;
    }
    size_t packet_size = mock_receive_sizes[mock_receive_index];
    memcpy(buffer, mock_receive_packets[mock_receive_index], packet_size);
    mock_receive_index++;
    return (ssize_t)packet_size;
  }
  return recv(socket_fd, buffer, length, flags);
}

int test_poll(struct pollfd *fds, nfds_t count, int timeout)
{
  if (mock_io_enabled)
  {
    (void)fds;
    (void)count;
    (void)timeout;
    if (mock_poll_result < 0)
    {
      errno = EIO;
    }
    return mock_poll_result;
  }
  return poll(fds, count, timeout);
}

int test_clock_gettime(clockid_t clock_id, struct timespec *time_value)
{
  if (mock_clock_enabled)
  {
    if (mock_clock_failure)
    {
      return -1;
    }
    (void)clock_id;
    mock_clock_calls++;
    time_value->tv_sec = mock_clock_calls <= 6 ? 1 : 3;
    time_value->tv_nsec = 0;
    return 0;
  }
  return clock_gettime(clock_id, time_value);
}

static void queue_wire_packet(PROTOCOL_TYPE type, uint32_t sequence,
                              const char *data, size_t data_length)
{
  uint8_t wire_packet[HEADER_SIZE + PAYLOAD_SIZE] = {0};

  wire_packet[0] = (uint8_t)type;
  wire_packet[4] = (uint8_t)(sequence >> 24);
  wire_packet[5] = (uint8_t)(sequence >> 16);
  wire_packet[6] = (uint8_t)(sequence >> 8);
  wire_packet[7] = (uint8_t)sequence;
  wire_packet[8] = (uint8_t)(data_length >> 8);
  wire_packet[9] = (uint8_t)data_length;
  if (data_length > 0)
  {
    memcpy(wire_packet + HEADER_SIZE, data, data_length);
  }
  uint16_t checksum = compute_checksum(wire_packet, HEADER_SIZE + data_length);
  wire_packet[2] = (uint8_t)(checksum >> 8);
  wire_packet[3] = (uint8_t)checksum;
  memcpy(mock_receive_packets[mock_receive_count], wire_packet, HEADER_SIZE + data_length);
  mock_receive_sizes[mock_receive_count] = HEADER_SIZE + data_length;
  mock_receive_count++;
}

void setUp(void)
{
  printf("Setting up tests...\n");
}

void tearDown(void)
{
  printf("Tearing down tests...\n");
}

void test_parse_cl_opt(void)
{
  char *argv[] = {"myapp", "-s", "client-session", "-p", "4300", "relay.example", "received.bin"};

  optind = 1;
  CLIENT_ARGUMENT *arguments = parse_cl_opt(7, argv);

  TEST_ASSERT_NOT_NULL(arguments);
  TEST_ASSERT_EQUAL_STRING("client-session", arguments->session);
  TEST_ASSERT_EQUAL_INT(4300, arguments->port);
  TEST_ASSERT_EQUAL_STRING("relay.example", arguments->relay);
  TEST_ASSERT_EQUAL_STRING("received.bin", arguments->file_name);

  free(arguments);
}

void test_parse_ser_opt(void)
{
  char *argv[] = {
      "myapp", "-s", "server-session", "-w", "16", "-T", "500",
      "-l", "10", "-c", "20", "-d", "30", "-p", "4300",
      "relay.example", "input.bin"};

  optind = 1;
  SERVER_ARGUMENT *arguments = parse_ser_opt(17, argv);

  TEST_ASSERT_NOT_NULL(arguments);
  TEST_ASSERT_EQUAL_STRING("server-session", arguments->session);
  TEST_ASSERT_EQUAL_INT(16, arguments->window);
  TEST_ASSERT_EQUAL_INT(500, arguments->timeout_ms);
  TEST_ASSERT_EQUAL_INT(10, arguments->loss);
  TEST_ASSERT_EQUAL_INT(20, arguments->corrupt);
  TEST_ASSERT_EQUAL_INT(30, arguments->dup);
  TEST_ASSERT_EQUAL_INT(4300, arguments->port);
  TEST_ASSERT_EQUAL_STRING("relay.example", arguments->relay);
  TEST_ASSERT_EQUAL_STRING("input.bin", arguments->file_name);

  free(arguments);
}

void test_compute_checksum(void)
{
  const uint8_t one_word[] = {0x00, 0x01};
  const uint8_t odd_length[] = {0x12, 0x34, 0x56};

  TEST_ASSERT_EQUAL_UINT16(0xffff, compute_checksum(NULL, 0));
  TEST_ASSERT_EQUAL_UINT16(0xfffe, compute_checksum(one_word, sizeof(one_word)));
  TEST_ASSERT_EQUAL_UINT16(0x97cb, compute_checksum(odd_length, sizeof(odd_length)));
}

void test_session_validator(void)
{
  TEST_ASSERT_TRUE(session_validator("valid-session-1"));
  TEST_ASSERT_TRUE(session_validator("a"));
  TEST_ASSERT_FALSE(session_validator(NULL));
  TEST_ASSERT_FALSE(session_validator("UpperCase"));
  TEST_ASSERT_FALSE(session_validator("has space"));
  TEST_ASSERT_FALSE(session_validator("this-session-name-is-longer-than-thirty-two"));
}

void test_is_relay_addr_valid(void)
{
  TEST_ASSERT_FALSE(is_relay_addr_valid(NULL, RELAY_PORT));
  TEST_ASSERT_FALSE(is_relay_addr_valid("127.0.0.1", 0));
  TEST_ASSERT_FALSE(is_relay_addr_valid("127.0.0.1", RELAY_PORT));
}

void test_parse_incoming_packet(void)
{
  uint8_t packet[HEADER_SIZE + 3] = {DATA, 0, 0, 0, 0, 0, 0, 7, 0, 3, 'a', 'b', 'c'};
  packet_header output;
  uint16_t checksum = compute_checksum(packet, sizeof(packet));
  packet[2] = (uint8_t)(checksum >> 8);
  packet[3] = (uint8_t)checksum;

  TEST_ASSERT_EQUAL_INT(0, parse_incoming_packet(packet, sizeof(packet), &output));
  TEST_ASSERT_EQUAL_INT(DATA, output.pack_type);
  TEST_ASSERT_EQUAL_UINT32(7, output.seq_num);
  TEST_ASSERT_EQUAL_UINT(3, output.data_len);
  TEST_ASSERT_EQUAL_MEMORY("abc", output.data, 3);
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(NULL, sizeof(packet), &output));
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(packet, HEADER_SIZE - 1, &output));
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(packet, sizeof(packet), NULL));

  packet[1] = 1;
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(packet, sizeof(packet), &output));
}

void test_packet_sending(void)
{
  int sockets[2];
  packet_header packet = {.pack_type = DATA, .seq_num = 4, .data_len = 0};
  uint8_t received[HEADER_SIZE];

  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  TEST_ASSERT_EQUAL_INT(0, send_packet(sockets[0], &packet));
  TEST_ASSERT_EQUAL_INT(HEADER_SIZE, recv(sockets[1], received, sizeof(received), 0));
  TEST_ASSERT_EQUAL_INT(DATA, received[0]);
  TEST_ASSERT_EQUAL_INT(4, received[7]);
  TEST_ASSERT_EQUAL_INT(0, parse_incoming_packet(received, sizeof(received), &packet));
  close(sockets[0]);
  close(sockets[1]);
}

void test_read_file_and_get_time(void)
{
  char path[] = "/tmp/cs425-test-XXXXXX";
  const char contents[] = "file contents";
  int file_fd = mkstemp(path);
  FILE_METADATA *metadata;
  int64_t first_time;

  TEST_ASSERT_TRUE(file_fd >= 0);
  TEST_ASSERT_EQUAL_INT((int)strlen(contents), (int)write(file_fd, contents, strlen(contents)));
  close(file_fd);

  metadata = read_file(path);
  TEST_ASSERT_NOT_NULL(metadata);
  TEST_ASSERT_EQUAL_UINT(strlen(contents), metadata->size);
  TEST_ASSERT_EQUAL_MEMORY(contents, metadata->data, strlen(contents));
  free(metadata->data);
  free(metadata);
  unlink(path);

  unlink("/tmp/file-does-not-exist-cs425");
  TEST_ASSERT_NULL(read_file("/tmp/file-does-not-exist-cs425"));
  first_time = get_time_ms();
  TEST_ASSERT_TRUE(first_time >= 0);
  TEST_ASSERT_TRUE(get_time_ms() >= first_time);
}

void test_parse_options_invalid(void)
{
  char *client_argv[] = {"myapp", "relay"};
  char *server_argv[] = {"myapp", "relay", "file"};
  char *bad_client_argv[] = {"myapp", "-x", "relay", "file"};
  char *bad_server_argv[] = {"myapp", "-x", "relay", "file"};

  optind = 1;
  TEST_ASSERT_NULL(parse_cl_opt(2, client_argv));
  optind = 1;
  TEST_ASSERT_NULL(parse_ser_opt(3, server_argv));
  optind = 1;
  TEST_ASSERT_NULL(parse_cl_opt(4, bad_client_argv));
  optind = 1;
  TEST_ASSERT_NULL(parse_ser_opt(4, bad_server_argv));
  TEST_ASSERT_NULL(parse_cl_opt(0, NULL));
  TEST_ASSERT_NULL(parse_ser_opt(0, NULL));
}

void test_network_initialization_and_registration(void)
{
  CLIENT_ARGUMENT client = {.session = "client", .relay = "localhost", .file_name = "file", .port = RELAY_PORT};
  SERVER_ARGUMENT server = {.session = "server", .relay = "localhost", .file_name = "file", .port = RELAY_PORT};
  CLIENT_ARGUMENT invalid_client = client;
  SERVER_ARGUMENT invalid_server = server;
  int sockets[2];
  const char reply[] = "OK";

  invalid_client.relay = "invalid host name";
  invalid_server.relay = "invalid host name";
  TEST_ASSERT_EQUAL_INT(1, init_client(&invalid_client));
  TEST_ASSERT_EQUAL_INT(1, init_server(&invalid_server));

  int client_fd = init_client(&client);
  int server_fd = init_server(&server);
  TEST_ASSERT_TRUE(client_fd >= 0);
  TEST_ASSERT_TRUE(server_fd >= 0);
  close(client_fd);
  close(server_fd);

  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  TEST_ASSERT_EQUAL_INT((int)sizeof(reply), (int)send(sockets[1], reply, sizeof(reply), 0));
  TEST_ASSERT_EQUAL_INT(0, register_client(sockets[0], &client));
  close(sockets[0]);
  close(sockets[1]);

  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  TEST_ASSERT_EQUAL_INT((int)sizeof(reply), (int)send(sockets[1], reply, sizeof(reply), 0));
  TEST_ASSERT_EQUAL_INT(0, register_server(sockets[0], &server));
  close(sockets[0]);
  close(sockets[1]);

  invalid_client.session = "INVALID";
  invalid_server.session = "INVALID";
  TEST_ASSERT_EQUAL_INT(-1, register_client(-1, &invalid_client));
  TEST_ASSERT_EQUAL_INT(-1, register_server(-1, &invalid_server));
  TEST_ASSERT_EQUAL_INT(-1, register_client(-1, &client));
  TEST_ASSERT_EQUAL_INT(-1, register_server(-1, &server));

  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  close(sockets[1]);
  TEST_ASSERT_EQUAL_INT(-1, register_client(sockets[0], &client));
  close(sockets[0]);

  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  close(sockets[1]);
  TEST_ASSERT_EQUAL_INT(-1, register_server(sockets[0], &server));
  close(sockets[0]);
}

void test_timeout_and_consume(void)
{
  client_state state = {.expected = 3, .finished = 0, .last_valid_ms = 1000};
  packet_header packet = {.pack_type = DATA, .seq_num = 3, .data_len = 0};
  FILE *file = tmpfile();
  int sockets[2];
  uint8_t ack[HEADER_SIZE];

  TEST_ASSERT_EQUAL_INT(30000, get_remaining_timeout_ms(&state, 1000));
  TEST_ASSERT_EQUAL_INT(0, get_remaining_timeout_ms(&state, 31000));
  TEST_ASSERT_EQUAL_INT(0, get_remaining_timeout_ms(NULL, 0));
  TEST_ASSERT_NOT_NULL(file);
  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  TEST_ASSERT_EQUAL_INT(0, consume(sockets[0], &state, &packet, file));
  TEST_ASSERT_EQUAL_UINT32(4, state.expected);
  TEST_ASSERT_EQUAL_INT(HEADER_SIZE, recv(sockets[1], ack, sizeof(ack), 0));
  TEST_ASSERT_EQUAL_INT(ACK, ack[0]);
  fclose(file);
  close(sockets[0]);
  close(sockets[1]);
}

void test_sender_state_machine(void)
{
  const uint8_t data[] = "payload";
  server_state state = {.data = data, .size = sizeof(data) - 1, .total_chunks = 1, .window = 2, .timeout_ms = 25};
  packet_header packets[WINDOW_MAX];
  packet_header ack = {.pack_type = ACK, .seq_num = 1, .data_len = 0};

  TEST_ASSERT_EQUAL_UINT(1, populate_transmission_window(&state, 100, packets));
  TEST_ASSERT_EQUAL_INT(DATA, packets[0].pack_type);
  TEST_ASSERT_EQUAL_UINT(7, packets[0].data_len);
  TEST_ASSERT_EQUAL_INT(1, handle_valid_ack(&state, &ack, 110));
  TEST_ASSERT_EQUAL_UINT32(1, state.base);
  TEST_ASSERT_EQUAL_UINT(1, populate_transmission_window(&state, 120, packets));
  TEST_ASSERT_EQUAL_INT(FIN, packets[0].pack_type);
  ack.seq_num = 2;
  TEST_ASSERT_EQUAL_INT(1, handle_valid_ack(&state, &ack, 130));
  TEST_ASSERT_TRUE(state.finished);
}

void test_protocol_error_branches(void)
{
  uint8_t packet[HEADER_SIZE] = {FIN, 0, 0, 0, 0, 0, 0, 1, 0, 0};
  packet_header output;
  packet_header packet_header_value = {.pack_type = DATA, .seq_num = 1, .data_len = 0};
  int sockets[2];

  packet[0] = 9;
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(packet, sizeof(packet), &output));
  packet[0] = FIN;
  packet[9] = 1;
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(packet, sizeof(packet) + 1, &output));
  packet[9] = 0;
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(packet, sizeof(packet), &output));
  packet[2] = 0xff;
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(packet, sizeof(packet), &output));

  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  close(sockets[1]);
  TEST_ASSERT_EQUAL_INT(-1, send_packet(sockets[0], &packet_header_value));
  TEST_ASSERT_EQUAL_INT(-1, send_ack(sockets[0], &packet_header_value));
  close(sockets[0]);
}

void test_consume_all_packet_cases(void)
{
  client_state state = {.expected = 1};
  packet_header data = {.pack_type = DATA, .seq_num = 1, .data_len = 3, .data = "abc"};
  packet_header duplicate = {.pack_type = DATA, .seq_num = 0, .data_len = 0};
  packet_header fin = {.pack_type = FIN, .seq_num = 2, .data_len = 0};
  FILE *file = tmpfile();
  int sockets[2];
  uint8_t ack[HEADER_SIZE];

  TEST_ASSERT_NOT_NULL(file);
  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  TEST_ASSERT_EQUAL_INT(0, consume(sockets[0], &state, &data, file));
  TEST_ASSERT_EQUAL_INT(3, (int)ftell(file));
  recv(sockets[1], ack, sizeof(ack), 0);
  TEST_ASSERT_EQUAL_INT(0, consume(sockets[0], &state, &duplicate, file));
  recv(sockets[1], ack, sizeof(ack), 0);
  TEST_ASSERT_EQUAL_INT(0, consume(sockets[0], &state, &fin, file));
  recv(sockets[1], ack, sizeof(ack), 0);
  TEST_ASSERT_TRUE(state.finished);
  TEST_ASSERT_EQUAL_INT(0, consume(sockets[0], &state, &fin, NULL));
  recv(sockets[1], ack, sizeof(ack), 0);
  fclose(file);
  close(sockets[0]);
  close(sockets[1]);
}

void test_state_guards_and_timeout_paths(void)
{
  server_state state = {0};
  packet_header packets[WINDOW_MAX];
  packet_header ack = {.pack_type = ACK, .seq_num = 1, .data_len = 0};
  const uint8_t data[] = "x";

  state.data = data;
  state.size = 1;
  state.total_chunks = 1;
  state.window = 1;
  state.timeout_ms = 10;
  TEST_ASSERT_EQUAL_UINT(0, populate_transmission_window(NULL, 0, packets));
  TEST_ASSERT_EQUAL_UINT(0, populate_transmission_window(&state, 0, NULL));
  state.failed = 1;
  TEST_ASSERT_EQUAL_UINT(0, populate_transmission_window(&state, 0, packets));
  state.failed = 0;
  state.finished = 1;
  TEST_ASSERT_EQUAL_UINT(0, populate_transmission_window(&state, 0, packets));
  state.finished = 0;
  TEST_ASSERT_EQUAL_UINT(0, handle_retransmission_timeout(&state, 0, NULL));
  state.finished = 1;
  TEST_ASSERT_EQUAL_UINT(0, handle_retransmission_timeout(&state, 100, packets));
  state.finished = 0;
  state.base = state.next = 1;
  TEST_ASSERT_EQUAL_UINT(0, handle_retransmission_timeout(&state, 100, packets));
  state.base = 0;
  state.next = 1;
  state.limit_ms = 200;
  TEST_ASSERT_EQUAL_UINT(0, handle_retransmission_timeout(&state, 100, packets));
  state.limit_ms = 0;
  state.num_timeouts = 9;
  TEST_ASSERT_EQUAL_UINT(0, handle_retransmission_timeout(&state, 100, packets));
  TEST_ASSERT_TRUE(state.failed);
  TEST_ASSERT_EQUAL_INT(0, handle_valid_ack(NULL, &ack, 0));
  TEST_ASSERT_EQUAL_INT(0, handle_valid_ack(&state, NULL, 0));
  state.failed = 0;
  state.finished = 0;
  state.base = 1;
  state.next = 2;
  TEST_ASSERT_EQUAL_INT(0, handle_valid_ack(&state, &(packet_header){.pack_type = DATA}, 0));
  TEST_ASSERT_EQUAL_INT(0, handle_valid_ack(&state, &(packet_header){.pack_type = ACK, .data_len = 1}, 0));
  TEST_ASSERT_EQUAL_INT(0, handle_valid_ack(&state, &(packet_header){.pack_type = ACK, .seq_num = 1}, 0));
  TEST_ASSERT_EQUAL_INT(INT_MAX, get_remaining_timeout_ms(&(client_state){.finished = 1, .linger_time_ms = INT64_MAX}, 0));
}

void test_retransmission_and_flush(void)
{
  const uint8_t data[] = "data";
  server_state state = {.data = data, .size = sizeof(data) - 1, .total_chunks = 1, .window = 1, .timeout_ms = 10};
  packet_header packets[WINDOW_MAX];
  int sockets[2];

  TEST_ASSERT_EQUAL_UINT(1, populate_transmission_window(&state, 100, packets));
  TEST_ASSERT_EQUAL_UINT(1, handle_retransmission_timeout(&state, 111, packets));
  TEST_ASSERT_EQUAL_UINT(0, handle_retransmission_timeout(NULL, 111, packets));
  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  TEST_ASSERT_EQUAL_INT(0, flush(sockets[0], packets, 0));
  close(sockets[0]);
  close(sockets[1]);
}

void test_process_and_publish_invalid_inputs(void)
{
  CLIENT_ARGUMENT client = {.file_name = "/tmp/file-does-not-exist-cs425"};
  SERVER_ARGUMENT server = {.file_name = "/tmp/file-does-not-exist-cs425", .window = 1, .timeout_ms = 1};

  TEST_ASSERT_EQUAL_INT(2, process(-1, &client));
  TEST_ASSERT_EQUAL_INT(2, publish(-1, &server));
  unlink(client.file_name);
}

void test_publish_success(void)
{
  char path[] = "/tmp/cs425-publish-XXXXXX";
  const char contents[] = "payload";
  int file_fd = mkstemp(path);
  SERVER_ARGUMENT server = {.session = "server", .file_name = path, .window = 1, .timeout_ms = 1000};

  TEST_ASSERT_TRUE(file_fd >= 0);
  TEST_ASSERT_EQUAL_INT((int)strlen(contents), (int)write(file_fd, contents, strlen(contents)));
  close(file_fd);
  mock_receive_count = 0;
  mock_receive_index = 0;
  queue_wire_packet(ACK, 1, NULL, 0);
  queue_wire_packet(ACK, 2, NULL, 0);
  mock_io_enabled = 1;
  TEST_ASSERT_EQUAL_INT(0, publish(42, &server));
  mock_io_enabled = 0;
  unlink(path);
}

void test_process_success(void)
{
  char path[] = "/tmp/cs425-process-XXXXXX";
  CLIENT_ARGUMENT client = {.file_name = path};
  int file_fd = mkstemp(path);

  TEST_ASSERT_TRUE(file_fd >= 0);
  close(file_fd);
  mock_receive_count = 0;
  mock_receive_index = 0;
  queue_wire_packet(DATA, 0, "abc", 3);
  queue_wire_packet(FIN, 1, NULL, 0);
  mock_io_enabled = 1;
  mock_clock_enabled = 1;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(0, process(42, &client));
  mock_clock_enabled = 0;
  mock_io_enabled = 0;
  unlink(path);
}

void test_mocked_io_failures(void)
{
  CLIENT_ARGUMENT client = {.session = "client"};
  SERVER_ARGUMENT server = {.session = "server"};
  packet_header packet = {.pack_type = DATA, .data_len = 0};
  int sockets[2];

  mock_clock_enabled = 1;
  mock_clock_failure = 1;
  TEST_ASSERT_EQUAL_INT(-1, get_time_ms());
  mock_clock_failure = 0;
  mock_clock_enabled = 0;

  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  mock_io_enabled = 1;
  mock_send_failure = 1;
  TEST_ASSERT_EQUAL_INT(-1, register_client(sockets[0], &client));
  TEST_ASSERT_EQUAL_INT(-1, register_server(sockets[0], &server));
  TEST_ASSERT_EQUAL_INT(-1, flush(sockets[0], &packet, 1));
  mock_send_failure = 0;
  mock_io_enabled = 0;
  close(sockets[0]);
  close(sockets[1]);
}

void test_mocked_receive_and_publish_failures(void)
{
  char path[] = "/tmp/cs425-failure-XXXXXX";
  int file_fd = mkstemp(path);
  SERVER_ARGUMENT server = {.file_name = path, .window = 1, .timeout_ms = 1000};
  CLIENT_ARGUMENT client = {.file_name = path};

  TEST_ASSERT_TRUE(file_fd >= 0);
  close(file_fd);
  mock_io_enabled = 1;
  mock_recv_failure = 1;
  TEST_ASSERT_EQUAL_INT(2, process(42, &client));
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
  mock_recv_failure = 0;
  mock_io_enabled = 0;
  unlink(path);
}

void test_publish_poll_and_clock_failures(void)
{
  char path[] = "/tmp/cs425-publish-failure-XXXXXX";
  int file_fd = mkstemp(path);
  SERVER_ARGUMENT server = {.file_name = path, .window = 1, .timeout_ms = 1000};

  TEST_ASSERT_TRUE(file_fd >= 0);
  close(file_fd);
  mock_io_enabled = 1;
  mock_poll_result = -1;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
  mock_poll_result = 1;
  mock_clock_enabled = 1;
  mock_clock_failure = 1;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
  mock_clock_failure = 0;
  mock_clock_enabled = 0;
  mock_io_enabled = 0;
  unlink(path);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_parse_cl_opt);
  RUN_TEST(test_parse_ser_opt);
  RUN_TEST(test_compute_checksum);
  RUN_TEST(test_session_validator);
  RUN_TEST(test_is_relay_addr_valid);
  RUN_TEST(test_parse_incoming_packet);
  RUN_TEST(test_packet_sending);
  RUN_TEST(test_read_file_and_get_time);
  RUN_TEST(test_parse_options_invalid);
  RUN_TEST(test_network_initialization_and_registration);
  RUN_TEST(test_timeout_and_consume);
  RUN_TEST(test_sender_state_machine);
  RUN_TEST(test_protocol_error_branches);
  RUN_TEST(test_consume_all_packet_cases);
  RUN_TEST(test_state_guards_and_timeout_paths);
  RUN_TEST(test_retransmission_and_flush);
  RUN_TEST(test_process_and_publish_invalid_inputs);
  RUN_TEST(test_publish_success);
  RUN_TEST(test_process_success);
  RUN_TEST(test_mocked_io_failures);
  RUN_TEST(test_mocked_receive_and_publish_failures);
  RUN_TEST(test_publish_poll_and_clock_failures);
  return UNITY_END();
}
