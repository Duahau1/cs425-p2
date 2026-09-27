#include <stdlib.h>
#include <stdio.h>
#include "harness/unity.h"
#include "../src/lab.h"
#include "../src/utils.h"

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
  char *argv[] = {"myapp", "-s", "client-session", "relay.example", "received.bin"};

  optind = 1;
  CLIENT_ARGUMENT *arguments = parse_cl_opt(5, argv);

  TEST_ASSERT_NOT_NULL(arguments);
  TEST_ASSERT_EQUAL_STRING("client-session", arguments->session);
  TEST_ASSERT_EQUAL_INT(RELAY_PORT, arguments->port);
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
  TEST_ASSERT_EQUAL_INT(500, arguments->timeout);
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

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_parse_cl_opt);
  RUN_TEST(test_parse_ser_opt);
  RUN_TEST(test_compute_checksum);
  return UNITY_END();
}
