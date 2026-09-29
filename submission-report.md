# Submission Report

- Submission generated at 09/29/2026 at 03:25:01

- Machine info: Linux runnervmtr4k5 6.17.0-1022-azure #22-Ubuntu SMP Mon Jul 27 17:24:03 UTC 2026 x86_64 x86_64 x86_64 GNU/Linux

## Note to Students

Please read this report carefully before submission.
Ensure that all sections are complete and accurate.
Look for any errors in the build or test outputs.
If you find any issues, correct them before submitting.
Post any questions on the class discussion board for help.


---

## README

# Project 2: Reliable Data Transfer

- Name: Van Nguyen
- Email: vannguyen599@u.boisestate.edu
- Class: CS525

## Known Bugs or Issues

There are no issues that I have known of. Payload has been sent successfully with correct arguments
![Server/Client Image](scripts/test.png)

You can find in `test.txt` are some of the commands that had been used to test.
No crash and memory leaks have been found with test coverage for `lab.h`, `utils.h` is 100%.

## Experience

This project was personally challenging and took a lot of time, especially when debugging and designing the code for testability. I used AI to help clarify the requirements, but I also had to read documentation carefully and as I did not have a lot of experience with coding server/client in C. Keeping the Go-Back-N logic independent from I/O was another important design challenge. I have learnt a lot about design state machine when doing this kind of project as most of the info and guidance that I got was by reading the diagram that they had in the book. I also spent hours debugging a segmentation fault that turned out to be caused by an off-by-one error in the header size. Testing is one of the part that I used AI quite a bit as I have it built some testing harness for me so that I can reuse them throughout the tests.

## Design

The protocol is divided conceptually into three layers:

1. **Packets:** `compute_checksum()` and `parse_incoming_packet()` validate and
   decode packet bytes. Packet encoding builds the header, payload, and checksum
   that are sent over the wire.
2. **Go-Back-N state machines:** The sender tracks its `base`, `next` sequence,
   window, and retransmission deadline. `populate_transmission_window()`,
   `handle_valid_ack()`, and `handle_retransmission_timeout()` implement sending,
   cumulative ACK progress, and retransmission. The receiver tracks its expected
   sequence and FIN linger state in `consume()`. Time values are passed into the
   sender's transition helpers rather than read from a clock there.
3. **I/O:** `publish()` and the receiver's `process()` coordinate the socket,
   relay registration, polling, clock, and files. They turn network events and
   timer expirations into state-machine calls, then send packets or write received
   data as required.

My project only partially enforces this separation: packet parsing and
sender state helpers are separate, but `send_packet()` performs a socket send,
`consume()` sends ACKs and writes to a file, and `publish()`/`process()` combine
I/O loops with protocol transitions. That's the reason why you can see a lot of mocking that I did
inside of the `server.c`, `client.c`, and `utils.c` so I can avoid the I/O blocking call. 

## Results

| Window | Loss | Corrupt | Dup | Average time (s) | Throughput (KiB/s) |
| ------ | ---- | ------- | --- | ---------------- | ------------------ |
| 1      | 0    | 0       | 0   | 104.90           | 9.76               |
| 16     | 0    | 0       | 0   | 6.714            | 152.52             |
| 1      | 0.05 | 0       | 0   | 129.70           | 7.90               |
| 16     | 0.05 | 0       | 0   | 23.822           | 42.99              |

Throughput is calculated as file size divided by average transfer time. Since
the transferred file is 1 MiB (1024 KiB), throughput in KiB/s is `1024 / time`.

1.  **From the window 1, no loss run, compute the round trip time your sender actually saw. (It sent 1025 packets and waited one round trip for each.) The relay adds 100 ms. Where does the rest come from?**

        The observed round trip time was `104.90 s / 1025 = 0.10234 s` per packet. This is about `2.34 ms` more than the relay's 100 ms
    delay, due to sender and receiver processing, relay scheduling, and timing
    overhead.

2.  **Use that round trip time to explain the speedup at window 16. Is it close to 16 times? Why or why not?**

        The window-16 run was `104.90 / 6.714 = 15.62` times faster, close to the

    16-packet window's speedup.This allows the sender to have up to 16
    packets in flight instead of waiting for an ACK after each packet. The small
    difference from 16 comes from startup and final-packet handling, plus processing
    and scheduling overhead that the larger window cannot eliminate.

3.  **Why does 5% loss cost the window 16 run more than it costs the window 1 run? Think about what a timeout makes the sender resend in each case.**

        With Go-Back-N, a timeout resends all unacknowledged packets. At window 16, one

    lost packet can cause as many as 16 packets to be sent again,
    including packets the receiver may already have received. At window 1, only one
    packet is outstanding, so a timeout wastes less work, though stop-and-wait still
    has high baseline latency. The relative slowdown is about 255% at window 16
    versus 24% at window 1. In absolute time, the measured increases are 17.11 s
    and 24.80 s respectively, so the window-16 loss penalty is larger
    proportionally, not in added seconds for these measurements.

---


## Build Output

This section was generated by running `make all` in the project root directory.

```bash
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/server.c -o build/debug/server.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/utils.c -o build/debug/utils.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/client.c -o build/debug/client.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug/main.c.o
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address build/debug/server.c.o build/debug/utils.c.o build/debug/client.c.o build/debug/main.c.o -o build/debug/myapp_d -fsanitize=address
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/server.c -o build/release/server.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/utils.c -o build/release/utils.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/client.c -o build/release/client.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/main.c -o build/release/main.c.o
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion build/release/server.c.o build/release/utils.c.o build/release/client.c.o build/release/main.c.o -o build/release/myapp 
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/server.c -o build/tests/server.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/utils.c -o build/tests/utils.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/client.c -o build/tests/client.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/main.c -o build/tests/main.c.o
mkdir -p build/tests/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/lab-test.c -o build/tests/lab-test.c.o
mkdir -p build/tests/harness/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/harness/unity.c -o build/tests/harness/unity.c.o
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage build/tests/server.c.o build/tests/utils.c.o build/tests/client.c.o build/tests/main.c.o build/tests/lab-test.c.o build/tests/harness/unity.c.o -o build/tests/myapp_t -fprofile-arcs -ftest-coverage
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/server.c -o build/debug-test/server.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/utils.c -o build/debug-test/utils.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/client.c -o build/debug-test/client.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug-test/main.c.o
mkdir -p build/debug-test/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/lab-test.c -o build/debug-test/lab-test.c.o
mkdir -p build/debug-test/harness/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/harness/unity.c -o build/debug-test/harness/unity.c.o
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address build/debug-test/server.c.o build/debug-test/utils.c.o build/debug-test/client.c.o build/debug-test/main.c.o build/debug-test/lab-test.c.o build/debug-test/harness/unity.c.o -o build/debug-test/myapp_td -fsanitize=address
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
Builds completed. You can run the application with: ./build/release/myapp
You can run the debug build with: ./build/debug/myapp_d
You can run the test build with: ./build/tests/myapp_t
You can run the debug-test build with: ./build/debug-test/myapp_td
```

---

## Coverage Report

This section was generated by running `make report` in the project root directory.

```bash
fopen: No such file or directory
Error getting file position: Invalid argument
Error getting file position: Invalid argument
Memory allocation failed: Invalid argument
Memory allocation failed: Invalid argument
Error reading file: Invalid argument
Error: Expected exactly 2 positional arguments (<relay> and <file>), but got 1.
Usage: myapp -s <session> [-p port] <relay> <file>
myapp: invalid option -- 'x'
myapp: invalid option -- 'x'
Error: Failed to look up the provided relay addressError: Failed to look up the provided relay addressSend register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Relay registration exhaust all 5Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Relay registration exhaust all 5Send register message failed: Connection refused
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Relay registration exhaust all 5Send register message failed: Connection refused
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Relay registration exhaust all 5Error: Failed to look up the provided relay addressFailed to send packet: Connection refused
Failed to send ACK packet: Transport endpoint is not connected
Error in receiving data: Bad file descriptor
Failed to send packet: Bad file descriptor
Unable to process file: Permission denied
fopen: Permission denied
Failed to send packet: Input/output error
Not able to create sever state: Input/output error
Failed to send packet: Input/output error
Failed to send packet: Input/output error
Transfer short circuit after 10 retransmission.
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Idle mode waiting for data: Input/output error
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Relay registration exhaust all 5Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Relay registration exhaust all 5Failed to send packet: Input/output error
Set receive timeout failed: Input/output error
Set receive timeout failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Relay registration exhaust all 5Relay registration exhaust all 5Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Relay registration exhaust all 5Relay registration exhaust all 5Error in receiving data: Input/output error
Error in receiving data: Input/output error
Idle mode waiting for client ACK: Input/output error
Setting up tests...
Tearing down tests...
tests/lab-test.c:1305:test_parse_cl_opt:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1306:test_parse_ser_opt:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1307:test_compute_checksum:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1308:test_session_validator:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1309:test_registration_response_classifier:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1310:test_parse_incoming_packet:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1311:test_packet_sending:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1312:test_read_file_and_get_time:PASS
Setting up tests...
Warning: Hit End-of-File early. Read 3 of 10 bytes.
Tearing down tests...
tests/lab-test.c:1313:test_read_file_error_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1314:test_parse_options_invalid:PASS
Setting up tests...
Client received reply: OK, with reply_length: 2
Server received reply: OK
Tearing down tests...
tests/lab-test.c:1315:test_network_initialization_and_registration:PASS
Setting up tests...
Client received reply: OK, with reply_length: 2
Client failed with the reply reply: ERR denied, with reply_length: 10
Client failed with the reply reply: NO, with reply_length: 2
Server received reply: OK
Server failed with the reply: ERR denied
Server failed with the reply: NO
Tearing down tests...
tests/lab-test.c:1316:test_registration_response_handling:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1317:test_server_socket_initialization_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1318:test_timeout_and_consume:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1319:test_sender_state_machine:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1320:test_protocol_error_branches:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1321:test_consume_all_packet_cases:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1322:test_state_guards_and_timeout_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1323:test_retransmission_and_flush:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1324:test_process_and_publish_invalid_inputs:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1325:test_process_open_failure:PASS
Setting up tests...
Sent DATA packet: seq=0, payload=7 bytes
Received ACK: seq=1
Sent FIN packet: seq=1, payload=0 bytes
Received ACK: seq=2
Tearing down tests...
tests/lab-test.c:1326:test_publish_success:PASS
Setting up tests...
Sent DATA packet: seq=0, payload=1 bytes
Received ACK: seq=1
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Tearing down tests...
tests/lab-test.c:1327:test_publish_error_and_timeout_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1328:test_process_success:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1329:test_process_idle_timeout:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1330:test_process_uncovered_timeout_and_packet_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1331:test_mocked_io_failures:PASS
Setting up tests...
Sent FIN packet: seq=0, payload=0 bytes
Tearing down tests...
tests/lab-test.c:1332:test_mocked_receive_and_publish_failures:PASS
Setting up tests...
Sent FIN packet: seq=0, payload=0 bytes
Tearing down tests...
tests/lab-test.c:1333:test_publish_poll_and_clock_failures:PASS

-----------------------
29 Tests 0 Failures 0 Ignored 
OK
./build/tests/myapp_t
fopen: No such file or directory
Error getting file position: Invalid argument
Error getting file position: Invalid argument
Memory allocation failed: Invalid argument
Memory allocation failed: Invalid argument
Error reading file: Invalid argument
Error: Expected exactly 2 positional arguments (<relay> and <file>), but got 1.
Usage: myapp -s <session> [-p port] <relay> <file>
myapp: invalid option -- 'x'
myapp: invalid option -- 'x'
Error: Failed to look up the provided relay addressError: Failed to look up the provided relay addressSend register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Relay registration exhaust all 5Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Relay registration exhaust all 5Send register message failed: Connection refused
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Relay registration exhaust all 5Send register message failed: Connection refused
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Relay registration exhaust all 5Error: Failed to look up the provided relay addressFailed to send packet: Connection refused
Failed to send ACK packet: Transport endpoint is not connected
Error in receiving data: Bad file descriptor
Failed to send packet: Bad file descriptor
Unable to process file: Permission denied
fopen: Permission denied
Failed to send packet: Input/output error
Not able to create sever state: Input/output error
Failed to send packet: Input/output error
Failed to send packet: Input/output error
Transfer short circuit after 10 retransmission.
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Idle mode waiting for data: Input/output error
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Relay registration exhaust all 5Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Relay registration exhaust all 5Failed to send packet: Input/output error
Set receive timeout failed: Input/output error
Set receive timeout failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Relay registration exhaust all 5Relay registration exhaust all 5Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Relay registration exhaust all 5Relay registration exhaust all 5Error in receiving data: Input/output error
Error in receiving data: Input/output error
Idle mode waiting for client ACK: Input/output error
Setting up tests...
Tearing down tests...
tests/lab-test.c:1305:test_parse_cl_opt:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1306:test_parse_ser_opt:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1307:test_compute_checksum:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1308:test_session_validator:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1309:test_registration_response_classifier:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1310:test_parse_incoming_packet:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1311:test_packet_sending:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1312:test_read_file_and_get_time:PASS
Setting up tests...
Warning: Hit End-of-File early. Read 3 of 10 bytes.
Tearing down tests...
tests/lab-test.c:1313:test_read_file_error_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1314:test_parse_options_invalid:PASS
Setting up tests...
Client received reply: OK, with reply_length: 2
Server received reply: OK
Tearing down tests...
tests/lab-test.c:1315:test_network_initialization_and_registration:PASS
Setting up tests...
Client received reply: OK, with reply_length: 2
Client failed with the reply reply: ERR denied, with reply_length: 10
Client failed with the reply reply: NO, with reply_length: 2
Server received reply: OK
Server failed with the reply: ERR denied
Server failed with the reply: NO
Tearing down tests...
tests/lab-test.c:1316:test_registration_response_handling:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1317:test_server_socket_initialization_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1318:test_timeout_and_consume:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1319:test_sender_state_machine:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1320:test_protocol_error_branches:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1321:test_consume_all_packet_cases:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1322:test_state_guards_and_timeout_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1323:test_retransmission_and_flush:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1324:test_process_and_publish_invalid_inputs:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1325:test_process_open_failure:PASS
Setting up tests...
Sent DATA packet: seq=0, payload=7 bytes
Received ACK: seq=1
Sent FIN packet: seq=1, payload=0 bytes
Received ACK: seq=2
Tearing down tests...
tests/lab-test.c:1326:test_publish_success:PASS
Setting up tests...
Sent DATA packet: seq=0, payload=1 bytes
Received ACK: seq=1
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Tearing down tests...
tests/lab-test.c:1327:test_publish_error_and_timeout_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1328:test_process_success:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1329:test_process_idle_timeout:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1330:test_process_uncovered_timeout_and_packet_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1331:test_mocked_io_failures:PASS
Setting up tests...
Sent FIN packet: seq=0, payload=0 bytes
Tearing down tests...
tests/lab-test.c:1332:test_mocked_receive_and_publish_failures:PASS
Setting up tests...
Sent FIN packet: seq=0, payload=0 bytes
Tearing down tests...
tests/lab-test.c:1333:test_publish_poll_and_clock_failures:PASS

-----------------------
29 Tests 0 Failures 0 Ignored 
OK
mkdir -p ./build/report/html
mkdir -p ./build/report/txt
gcovr -r . --html --html-details --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$' -o ./build/report/html/coverage_report.html
(INFO) Reading coverage data...

(INFO) Writing coverage report...

gcovr -r . --txt                 --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$'
(INFO) Reading coverage data...

(INFO) Writing coverage report...

------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: .
------------------------------------------------------------------------------
File                                       Lines     Exec  Cover   Missing
------------------------------------------------------------------------------
src/client.c                                 166      166   100%
src/server.c                                 242      242   100%
src/utils.c                                  118      118   100%
------------------------------------------------------------------------------
TOTAL                                        526      526   100%
------------------------------------------------------------------------------
```

---

## Address Sanitizer Report

This section was generated by running `make leak-test` in the project root directory.

```bash
fopen: No such file or directory
Error getting file position: Invalid argument
Error getting file position: Invalid argument
Memory allocation failed: Invalid argument
Memory allocation failed: Invalid argument
Error reading file: Invalid argument
Error: Expected exactly 2 positional arguments (<relay> and <file>), but got 1.
Usage: myapp -s <session> [-p port] <relay> <file>
myapp: invalid option -- 'x'
myapp: invalid option -- 'x'
Error: Failed to look up the provided relay addressError: Failed to look up the provided relay addressSend register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Relay registration exhaust all 5Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Send register message failed: Bad file descriptor
Relay registration exhaust all 5Send register message failed: Connection refused
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Relay registration exhaust all 5Send register message failed: Connection refused
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Send register message failed: Transport endpoint is not connected
Relay registration exhaust all 5Error: Failed to look up the provided relay addressFailed to send packet: Connection refused
Failed to send ACK packet: Transport endpoint is not connected
Error in receiving data: Bad file descriptor
Failed to send packet: Bad file descriptor
Unable to process file: Permission denied
fopen: Permission denied
Failed to send packet: Input/output error
Not able to create sever state: Input/output error
Failed to send packet: Input/output error
Failed to send packet: Input/output error
Transfer short circuit after 10 retransmission.
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Idle mode waiting for data: Input/output error
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Receiver timed outputHeader after 30 seconds idle.
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Relay registration exhaust all 5Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Send register message failed: Input/output error
Relay registration exhaust all 5Failed to send packet: Input/output error
Set receive timeout failed: Input/output error
Set receive timeout failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Relay registration exhaust all 5Relay registration exhaust all 5Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Receive register response failed: Input/output error
Relay registration exhaust all 5Relay registration exhaust all 5Error in receiving data: Input/output error
Error in receiving data: Input/output error
Idle mode waiting for client ACK: Input/output error
Setting up tests...
Tearing down tests...
tests/lab-test.c:1305:test_parse_cl_opt:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1306:test_parse_ser_opt:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1307:test_compute_checksum:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1308:test_session_validator:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1309:test_registration_response_classifier:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1310:test_parse_incoming_packet:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1311:test_packet_sending:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1312:test_read_file_and_get_time:PASS
Setting up tests...
Warning: Hit End-of-File early. Read 3 of 10 bytes.
Tearing down tests...
tests/lab-test.c:1313:test_read_file_error_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1314:test_parse_options_invalid:PASS
Setting up tests...
Client received reply: OK, with reply_length: 2
Server received reply: OK
Tearing down tests...
tests/lab-test.c:1315:test_network_initialization_and_registration:PASS
Setting up tests...
Client received reply: OK, with reply_length: 2
Client failed with the reply reply: ERR denied, with reply_length: 10
Client failed with the reply reply: NO, with reply_length: 2
Server received reply: OK
Server failed with the reply: ERR denied
Server failed with the reply: NO
Tearing down tests...
tests/lab-test.c:1316:test_registration_response_handling:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1317:test_server_socket_initialization_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1318:test_timeout_and_consume:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1319:test_sender_state_machine:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1320:test_protocol_error_branches:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1321:test_consume_all_packet_cases:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1322:test_state_guards_and_timeout_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1323:test_retransmission_and_flush:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1324:test_process_and_publish_invalid_inputs:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1325:test_process_open_failure:PASS
Setting up tests...
Sent DATA packet: seq=0, payload=7 bytes
Received ACK: seq=1
Sent FIN packet: seq=1, payload=0 bytes
Received ACK: seq=2
Tearing down tests...
tests/lab-test.c:1326:test_publish_success:PASS
Setting up tests...
Sent DATA packet: seq=0, payload=1 bytes
Received ACK: seq=1
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Sent DATA packet: seq=0, payload=1 bytes
Tearing down tests...
tests/lab-test.c:1327:test_publish_error_and_timeout_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1328:test_process_success:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1329:test_process_idle_timeout:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1330:test_process_uncovered_timeout_and_packet_paths:PASS
Setting up tests...
Tearing down tests...
tests/lab-test.c:1331:test_mocked_io_failures:PASS
Setting up tests...
Sent FIN packet: seq=0, payload=0 bytes
Tearing down tests...
tests/lab-test.c:1332:test_mocked_receive_and_publish_failures:PASS
Setting up tests...
Sent FIN packet: seq=0, payload=0 bytes
Tearing down tests...
tests/lab-test.c:1333:test_publish_poll_and_clock_failures:PASS

-----------------------
29 Tests 0 Failures 0 Ignored 
OK
```

---

## Src Files
### client.c

```c

#include "lab.h"
#include "utils.h"
#include <errno.h>
#include <netdb.h>

#ifdef TEST
extern ssize_t test_send(int socket_fd, const void *buffer, size_t length, int flags);
extern ssize_t test_recv(int socket_fd, void *buffer, size_t length, int flags);
extern int test_socket(int domain, int type, int protocol);
extern int test_connect(int socket_fd, const struct sockaddr *addr, socklen_t addr_len);
extern int test_setsockopt(int socket_fd, int level, int option_name,
                           const void *option_value, socklen_t option_length);
extern FILE *test_fopen(const char *path, const char *mode);
extern int test_get_remaining_timeout_ms(const client_state *state, int64_t now);
#define send test_send
#define recv test_recv
#define socket test_socket
#define connect test_connect
#define setsockopt test_setsockopt
#define fopen test_fopen
#define get_remaining_timeout_ms test_get_remaining_timeout_ms
#endif

CLIENT_ARGUMENT *parse_cl_opt(int argc, char *const argv[])
{

    int option;
    char *session = NULL;
    char *relay = NULL;
    char *file_name = NULL;
    int port = RELAY_PORT;

    CLIENT_ARGUMENT *clientArgument = malloc(sizeof(*clientArgument));

    if (argc < 1 || argv == NULL || clientArgument == NULL)
    {
        free(clientArgument);
        return NULL;
    }

    while ((option = getopt(argc, argv, "s:p:")) != -1)
    {
        if (option == '?' || option == ':')
        {
            free(clientArgument);
            return NULL;
        }
        switch (option)
        {
        case 's':
            session = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        }
    }
    int remaining_args = argc - optind;

    if (remaining_args != 2)
    {
        fprintf(stderr, "Error: Expected exactly 2 positional arguments (<relay> and <file>), but got %d.\n", remaining_args);
        fprintf(stderr, "Usage: %s -s <session> [-p port] <relay> <file>\n", argv[0]);
        free(clientArgument);
        return NULL;
    }

    relay = argv[optind];
    file_name = argv[optind + 1];

    clientArgument->session = session;
    clientArgument->port = port;
    clientArgument->relay = relay;
    clientArgument->file_name = file_name;

    return clientArgument;
}

int init_client(CLIENT_ARGUMENT *client)
{
    int sock_fd = -1;
    struct addrinfo resolver;
    struct addrinfo *result = NULL;
    struct addrinfo *rp = NULL;

    memset(&resolver, 0, sizeof(resolver));
    resolver.ai_family = AF_UNSPEC;
    resolver.ai_socktype = SOCK_DGRAM;
    resolver.ai_protocol = IPPROTO_UDP;

    char port_string[INET6_ADDRSTRLEN];
    snprintf(port_string, sizeof(port_string), "%d", client->port);
    int status = getaddrinfo(client->relay, port_string, &resolver, &result);

    if (status != 0)
    {
        fprintf(stderr, "Error: Failed to look up the provided relay address");
        return 1;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sock_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock_fd == -1)
        {
            continue;
        }

        if (connect(sock_fd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }

        close(sock_fd);
        sock_fd = -1;
    }

    freeaddrinfo(result);
    return sock_fd;
}

int register_client(int socket_fd, CLIENT_ARGUMENT *client)
{
    char register_message[250];
    char reply[250];

    struct timeval receive_timeout = {
        .tv_sec = 1,
        .tv_usec = 0};

    if (session_validator(client->session) == 0)
    {
        return -1;
    }
    snprintf(register_message, sizeof(register_message), "HELLO %s recv", client->session);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO,
                   &receive_timeout, sizeof(receive_timeout)) < 0)
    {
        perror("Set receive timeout failed");
        return -1;
    }

    for (int attempt = 0; attempt < REGISTER_MAX_ATTEMPT; ++attempt)
    {
        if (send(socket_fd, register_message, strlen(register_message), 0) < 0)
        {
            perror("Send register message failed");
            continue;
        }

        ssize_t reply_length = recv(socket_fd, reply, sizeof(reply) - 1, 0);
        if (reply_length >= 0)
        {
            reply[reply_length] = '\0';
            registration_status register_status = evaluate_registration_response(reply, (size_t)reply_length);
            if (register_status == REG_FAILURE || register_status == REG_MALFORMED)
            {
                printf("Client failed with the reply reply: %s, with reply_length: %zd\n", reply, reply_length);
                return 2;
            }
            printf("Client received reply: %s, with reply_length: %zd\n", reply, reply_length);
            return 0;
        }

        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("Receive register response failed");
        }
    }
    fprintf(stderr, "Relay registration exhaust all %d", REGISTER_MAX_ATTEMPT);
    return -1;
}

int process(int fd, CLIENT_ARGUMENT *client)
{
    int returnCode = 2;
    FILE *file = fopen(client->file_name, "wb");
    if (file == NULL)
    {
        perror("Unable to process file");
        return returnCode;
    }
    client_state current_state = {0};
    int64_t now = get_time_ms();

    current_state.last_valid_ms = now;
    while (1)
    {
        now = get_time_ms();
        if (current_state.finished && now >= current_state.linger_time_ms)
        {
            returnCode = 0;
            break;
        }
        else
        {
            if (now - current_state.last_valid_ms >= 30000)
            {

                fprintf(stderr, "Receiver timed outputHeader after 30 seconds idle.\n");
                break;
            }
        }
        int remaining_timeout = get_remaining_timeout_ms(&current_state, now);
        if (remaining_timeout < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("Idle mode waiting for data");
            break;
        }
        if (remaining_timeout == 0)
        {
            continue;
        }
        uint8_t payloadBuffer[PAYLOAD_SIZE + PAYLOAD_SIZE];
        ssize_t receivedBytes = recv(fd, payloadBuffer, sizeof(payloadBuffer), 0);
        if (receivedBytes < 0)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            perror("Error in receiving data");
            break;
        }
        packet_header incoming_header;
        int parse_packet = parse_incoming_packet(payloadBuffer, (size_t)receivedBytes, &incoming_header);
        if (parse_packet != 0)
        {
            continue;
        }
        consume(fd, &current_state, &incoming_header, file);
    }
    fclose(file);
    return returnCode;
}

#ifdef TEST
#undef get_remaining_timeout_ms
#endif
int get_remaining_timeout_ms(const client_state *state, int64_t now)
{
    if (state == NULL)
    {
        return 0;
    }

    int64_t deadline = state->linger_time_ms;
    if (!state->finished)
    {
        const int64_t INACTIVITY_TIMEOUT_MS = 30000;
        deadline = state->last_valid_ms + INACTIVITY_TIMEOUT_MS;
    }

    int64_t remaining = deadline - now;
    if (remaining <= 0)
    {
        return 0;
    }

    // Safely clamp to INT_MAX to prevent 32-bit truncation errors during type casting
    if (remaining > INT_MAX)
    {
        return INT_MAX;
    }

    return (int)remaining;
}

int consume(int sock_fd, client_state *client_state, packet_header *incoming_packet, FILE *opened_file)
{
    if (client_state == NULL || incoming_packet == NULL || incoming_packet->data_len > PAYLOAD_SIZE ||
        (incoming_packet->pack_type == DATA && incoming_packet->data_len > 0 && opened_file == NULL))
    {
        return -1;
    }

    client_state->last_valid_ms = get_time_ms();

    // Case 1: In-order DATA packet
    if (client_state->finished == 0 && incoming_packet->pack_type == DATA && incoming_packet->seq_num == client_state->expected)
    {
        if (incoming_packet->data_len > 0)
        {
            fwrite(incoming_packet->data, 1, incoming_packet->data_len, opened_file);
        }
        client_state->expected++;
        send_ack(sock_fd, client_state->expected);
    }
    // Case 2: Out-of-order or duplicate DATA packet
    else if (incoming_packet->pack_type == DATA)
    {
        send_ack(sock_fd, client_state->expected);
    }
    // Case 3: In-order FIN packet
    else if (incoming_packet->pack_type == FIN && incoming_packet->seq_num == client_state->expected)
    {
        client_state->expected++;
        send_ack(sock_fd, client_state->expected);

        // Initiate the 2-second linger phase
        client_state->finished = 1;
        client_state->linger_time_ms = get_time_ms() + 2000;
    }
    // Case 4: Repeated FIN during the linger window
    else if (incoming_packet->pack_type == FIN && client_state->finished)
    {
        // Answer repeated FINs with the exact same ACK to help the sender close cleanly
        send_ack(sock_fd, client_state->expected);
    }
    return 0;
}
int send_ack(int sock_fd, uint32_t sequence_number)
{
    uint8_t ack_packet[HEADER_SIZE] = {0};

    ack_packet[0] = ACK;

    ack_packet[4] = (uint8_t)(sequence_number >> 24);
    ack_packet[5] = (uint8_t)(sequence_number >> 16);
    ack_packet[6] = (uint8_t)(sequence_number >> 8);
    ack_packet[7] = (uint8_t)sequence_number;

    uint16_t checksum = compute_checksum(ack_packet, sizeof(ack_packet));
    ack_packet[2] = (uint8_t)(checksum >> 8);
    ack_packet[3] = (uint8_t)checksum;
    ssize_t bytes_sent = send(sock_fd, ack_packet, sizeof(ack_packet), 0);
    if (bytes_sent < 0)
    {
        perror("Failed to send ACK packet");
        return -1;
    }
    return 0;
}

```

### lab.h

```c

#ifndef LAB_H
#define LAB_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>
#include <netdb.h>
#include <poll.h>
#include <limits.h>

#define MAX_SIZE_OF_FILE (16U * 1024U * 1024U)

#define PAYLOAD_SIZE 1024U
#define HEADER_SIZE 10U

#define TIMEOUT 250

#define RELAY_PORT 4250

#define REGISTER_MAX_ATTEMPT 5

#define WINDOW_MAX 64U

typedef enum
{
    DATA,
    ACK,
    FIN
} PROTOCOL_TYPE;

// Header structure for packet
typedef struct
{
    PROTOCOL_TYPE pack_type;
    uint16_t checksum;
    uint32_t seq_num;
    size_t data_len;
    char data[PAYLOAD_SIZE];
} packet_header;

typedef struct
{
    char *session;
    char *relay;
    char *file_name;
    int port;
} CLIENT_ARGUMENT;

typedef struct
{
    char *session;
    char *relay;
    char *file_name;
    int window;
    int timeout_ms;
    double loss;
    double corrupt;
    double dup;
    int port;
} SERVER_ARGUMENT;

typedef struct
{
    uint32_t expected;
    int finished;
    int64_t last_valid_ms;
    int64_t linger_time_ms;
} client_state;

CLIENT_ARGUMENT *parse_cl_opt(int argc, char *const argv[]);

int init_client(CLIENT_ARGUMENT *client);
int register_client(int fd, CLIENT_ARGUMENT *client);
int process(int fd, CLIENT_ARGUMENT *client);

typedef struct
{
    const uint8_t *data;
    size_t size;
    uint32_t total_chunks;
    uint32_t base;
    uint32_t next;
    int64_t limit_ms;
    unsigned window;
    unsigned timeout_ms;
    int finished;
    int failed;
    unsigned num_timeouts;
    packet_header standby[WINDOW_MAX];
} server_state;

SERVER_ARGUMENT *parse_ser_opt(int argc, char *const argv[]);

int init_server(SERVER_ARGUMENT *client);
int register_server(int fd, SERVER_ARGUMENT *server);
int publish(int fd, SERVER_ARGUMENT *server);
int process(int fd, CLIENT_ARGUMENT *client);
int get_remaining_timeout_ms(const client_state *state, int64_t now);
int consume(int sock_fd, client_state *client_state, packet_header *incoming_packet, FILE *opened_file);
int send_ack(int sock_fd, uint32_t sequence_number);
size_t populate_transmission_window(server_state *state, int64_t now_ms, packet_header *out);
int flush(int fd, const packet_header *packets, size_t count);
size_t handle_retransmission_timeout(server_state *state, int64_t now_ms, packet_header *out);
int handle_valid_ack(server_state *state, const packet_header *header, int64_t now_ms);

#endif // LAB_H

```

### main.c

```c

#include "utils.h"
#include "lab.h"

#ifdef TEST
#define main main_exclude
#endif

void print_manual(void)
{
    puts("Usage: myapp send -s <session> [-w window] [-T timeout-ms] [-l loss]");
    puts("                  [-c corrupt] [-d dup] [-p port] <relay> <file>");
    puts("       myapp recv -s <session> [-p port] <relay> <file>");
    puts("");
    puts("  -s <session>     session name shared by the sender and the receiver");
    puts("  -w <window>      Go-Back-N window size in packets, 1 to 64 (default: 8)");
    puts("  -T <timeout-ms>  retransmission timeout in milliseconds (default: 250)");
    puts("  -l <loss>        probability the relay drops a packet (default: 0)");
    puts("  -c <corrupt>     probability the relay flips a bit (default: 0)");
    puts("  -d <dup>         probability the relay duplicates a packet (default: 0)");
    puts("  -p <port>        relay port (default: 4250)");
    puts("  <relay>          host name or address of the relay");
    puts("  <file>           file to send, or file to write what is received");
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        print_manual();
        return 0;
    }

    char *parser_argv[argc - 1];
    parser_argv[0] = argv[0];
    for (int index = 1; index < argc - 1; index++)
    {
        parser_argv[index] = argv[index + 1];
    }

    if (strcmp(argv[1], "send") == 0)
    {
        SERVER_ARGUMENT *serverArgument = parse_ser_opt(argc - 1, parser_argv);

        // printf("send: session=%s, window=%d, timeout_ms=%d, loss=%d, corrupt=%d, dup=%d, port=%d, relay=%s, file=%s\n",
        //        serverArgument->session,
        //        serverArgument->window,
        //        serverArgument->timeout_ms,
        //        serverArgument->loss,
        //        serverArgument->corrupt,
        //        serverArgument->dup,
        //        serverArgument->port,
        //        serverArgument->relay,
        //        serverArgument->file_name);
        if (serverArgument == NULL || serverArgument->session == NULL ||
            serverArgument->relay == NULL || serverArgument->file_name == NULL)
        {
            print_manual();
            free(serverArgument);
            return 1;
        }
        int server_init_status = init_server(serverArgument);
        register_server(server_init_status, serverArgument);
        publish(server_init_status, serverArgument);
        close(server_init_status);
        free(serverArgument);
    }
    else if (strcmp(argv[1], "recv") == 0)
    {
        CLIENT_ARGUMENT *clientArgument = parse_cl_opt(argc - 1, parser_argv);
        // printf("recv: session=%s, port=%d, relay=%s, file=%s\n",
        //        clientArgument->session,
        //        clientArgument->port,
        //        clientArgument->relay,
        //        clientArgument->file_name);
        if (clientArgument == NULL || clientArgument->session == NULL ||
            clientArgument->relay == NULL || clientArgument->file_name == NULL)
        {
            print_manual();
            free(clientArgument);
            return 1;
        }
        int client_init_status = init_client(clientArgument);
        register_client(client_init_status, clientArgument);
        int process_status = process(client_init_status, clientArgument);
        if (process_status == 0)
        {
            printf("File received successfully: %s\n", clientArgument->file_name);
        }
        close(client_init_status);
        free(clientArgument);
        return process_status;
    }
    else
    {
        print_manual();
        return 0;
    }
    return 0;
}
```

### server.c

```c

#include "lab.h"
#include "utils.h"
#include <errno.h>

#ifdef TEST
extern ssize_t test_send(int socket_fd, const void *buffer, size_t length, int flags);
extern ssize_t test_recv(int socket_fd, void *buffer, size_t length, int flags);
extern int test_poll(struct pollfd *fds, nfds_t count, int timeout);
extern int test_socket(int domain, int type, int protocol);
extern int test_connect(int socket_fd, const struct sockaddr *addr, socklen_t addr_len);
extern int test_getaddrinfo(const char *node, const char *service,
                            const struct addrinfo *hints, struct addrinfo **result);
extern void test_freeaddrinfo(struct addrinfo *result);
extern int test_setsockopt(int socket_fd, int level, int option_name,
                           const void *option_value, socklen_t option_length);
extern int test_getopt(int argc, char *const argv[], const char *options);
extern void *test_malloc(size_t size);
#define send test_send
#define recv test_recv
#define poll test_poll
#define socket test_socket
#define connect test_connect
#define getaddrinfo test_getaddrinfo
#define freeaddrinfo test_freeaddrinfo
#define setsockopt test_setsockopt
#define getopt test_getopt
#define malloc test_malloc
#endif

SERVER_ARGUMENT *parse_ser_opt(int argc, char *const argv[])
{
    int option;
    SERVER_ARGUMENT *serverArgument = malloc(sizeof(*serverArgument));

    if (argc < 1 || argv == NULL || serverArgument == NULL)
    {
        free(serverArgument);
        return NULL;
    }

    serverArgument->session = NULL;
    serverArgument->relay = NULL;
    serverArgument->file_name = NULL;
    serverArgument->window = 8;
    serverArgument->timeout_ms = TIMEOUT;
    serverArgument->loss = 0;
    serverArgument->corrupt = 0;
    serverArgument->dup = 0;
    serverArgument->port = RELAY_PORT;

    while ((option = getopt(argc, argv, "s:w:T:l:c:d:p:")) != -1)
    {
        if (option == '?' || option == ':')
        {
            free(serverArgument);
            return NULL;
        }

        switch (option)
        {
        case 's':
            serverArgument->session = optarg;
            break;
        case 'w':
            serverArgument->window = atoi(optarg);
            break;
        case 'T':
            serverArgument->timeout_ms = atoi(optarg);
            break;
        case 'l':
            serverArgument->loss = strtod(optarg, NULL);
            break;
        case 'c':
            serverArgument->corrupt = strtod(optarg, NULL);
            break;
        case 'd':
            serverArgument->dup = strtod(optarg, NULL);
            break;
        case 'p':
            serverArgument->port = atoi(optarg);
            break;
        default:
            free(serverArgument);
            return NULL;
        }
    }

    if (serverArgument->session == NULL || argc - optind != 2)
    {
        free(serverArgument);
        return NULL;
    }

    serverArgument->relay = argv[optind];
    serverArgument->file_name = argv[optind + 1];

    return serverArgument;
}

int init_server(SERVER_ARGUMENT *server)
{
    int sock_fd = -1;
    struct addrinfo resolver;
    struct addrinfo *result = NULL;
    struct addrinfo *rp = NULL;

    memset(&resolver, 0, sizeof(resolver));
    resolver.ai_family = AF_UNSPEC;
    resolver.ai_socktype = SOCK_DGRAM;
    resolver.ai_protocol = IPPROTO_UDP;

    char port_string[INET6_ADDRSTRLEN];
    snprintf(port_string, sizeof(port_string), "%d", server->port);
    int status = getaddrinfo(server->relay, port_string, &resolver, &result);

    if (status != 0)
    {
        fprintf(stderr, "Error: Failed to look up the provided relay address");
        return 1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sock_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock_fd == -1)
        {
            continue;
        }

        if (connect(sock_fd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }

        close(sock_fd);
        sock_fd = -1;
    }

    freeaddrinfo(result);
    return sock_fd;
}

int register_server(int fd, SERVER_ARGUMENT *server)
{
    char init_message[250];
    char reply[250];
    struct timeval receive_timeout = {
        .tv_sec = 1,
        .tv_usec = 0};

    if (session_validator(server->session) == 0)
    {
        return -1;
    }
    snprintf(init_message, sizeof(init_message), "HELLO %s send %.3g %.3g %.3g",
             server->session, server->loss, server->corrupt, server->dup);

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                   &receive_timeout, sizeof(receive_timeout)) < 0)
    {
        perror("Set receive timeout failed");
        return -1;
    }

    for (int attempt = 0; attempt < REGISTER_MAX_ATTEMPT; ++attempt)
    {
        if (send(fd, init_message, strlen(init_message), 0) < 0)
        {
            perror("Send register message failed");
            continue;
        }

        ssize_t reply_length = recv(fd, reply, sizeof(reply) - 1, 0);
        if (reply_length >= 0)
        {
            reply[reply_length] = '\0';
            registration_status register_status = evaluate_registration_response(reply, (size_t)reply_length);

            if (register_status == REG_FAILURE || register_status == REG_MALFORMED)
            {
                printf("Server failed with the reply: %s\n", reply);
                return 2;
            }
            printf("Server received reply: %s\n", reply);
            return 0;
        }

        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("Receive register response failed");
        }
    }
    fprintf(stderr, "Relay registration exhaust all %d", REGISTER_MAX_ATTEMPT);
    return -1;
}

int publish(int fd, SERVER_ARGUMENT *server)
{
    int returnCode = 2;
    FILE_METADATA *fileMetadata = read_file(server->file_name);

    if (fileMetadata == NULL)
    {

        return returnCode;
    }
    packet_header packets[WINDOW_MAX];

    server_state *current_state = malloc(sizeof(*current_state));
    if (current_state == NULL)
    {
        perror("Not able to create sever state");
        free(fileMetadata->data);
        free(fileMetadata);
        return returnCode;
    }
    if ((unsigned)server->window > WINDOW_MAX || server->window < 1 || server->timeout_ms == 0)
    {
        free(current_state);
        free(fileMetadata->data);
        free(fileMetadata);
        return returnCode;
    }
    memset(current_state, 0, sizeof(*current_state));

    current_state->data = fileMetadata->data;
    current_state->size = fileMetadata->size;
    current_state->window = (unsigned)server->window;
    current_state->timeout_ms = (unsigned)server->timeout_ms;
    // Standard Division Rounds Down trick
    current_state->total_chunks = (uint32_t)((fileMetadata->size + PAYLOAD_SIZE - 1) /
                                             PAYLOAD_SIZE);
    int64_t now = get_time_ms();
    if (now < 0 || flush(fd, packets, populate_transmission_window(current_state, now, packets)) != 0)
    {
        free(current_state);
        free(fileMetadata->data);
        free(fileMetadata);
        return returnCode;
    }
    while (!current_state->finished && !current_state->failed)
    {
        now = get_time_ms();
        int64_t remaining_time_ms = current_state->limit_ms - now;
        if (remaining_time_ms <= 0)
        {
            size_t count = handle_retransmission_timeout(current_state, now, packets);
            if (current_state->failed)
            {
                fprintf(stderr, "Transfer short circuit after 10 retransmission.\n");
                break;
            }
            if (flush(fd, packets, count) != 0)
            {
                break;
            }
            continue;
        }
        struct pollfd pfd = {.fd = fd, .events = POLLIN};
        int remaining_timeout = poll(&pfd, 1, remaining_time_ms > INT_MAX ? INT_MAX : (int)remaining_time_ms);
        if (remaining_timeout < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("Idle mode waiting for client ACK");
            break;
        }
        if (remaining_timeout == 0)
        {
            continue;
        }
        uint8_t payloadBuffer[PAYLOAD_SIZE + PAYLOAD_SIZE];
        ssize_t receivedBytes = recv(fd, payloadBuffer, sizeof(payloadBuffer), 0);
        if (receivedBytes < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("Error in receiving data");
            break;
        }
        packet_header incoming_header;
        int parse_packet = parse_incoming_packet(payloadBuffer, (size_t)receivedBytes, &incoming_header);
        if (parse_packet != 0)
        {
            continue;
        }
        if (incoming_header.pack_type == ACK)
        {
            printf("Received ACK: seq=%u\n", (unsigned)incoming_header.seq_num);
        }
        now = get_time_ms();
        int handledValidAck = handle_valid_ack(current_state, &incoming_header, now);
        int handleFlush = flush(fd, packets, populate_transmission_window(current_state, now, packets));
        if (handledValidAck && !current_state->finished && handleFlush != 0)
        {
            break;
        }
    }
    if (current_state->finished)
    {
        returnCode = 0;
    }
    free(fileMetadata->data);
    free(fileMetadata);
    free(current_state);

    return returnCode;
}

/**
 * Populates the sliding window with fresh DATA packets up to the window capacity,
 * or generates a FIN packet if all payload data has been fully acknowledged.
 *
 * @param[in,out] state  Pointer to the sender's active state machine tracking object.
 * @param[in]     now_ms The current system timestamp in milliseconds (from a monotonic clock).
 * @param[out]    out    An allocated array where generated packets will be staged for transmission.
 * @return               The total number of packets successfully placed into the 'out' array.
 */
size_t populate_transmission_window(server_state *state, int64_t now_ms, packet_header *out)
{
    // 1. Guard clauses for invalid states or completed operations
    if (state == NULL || out == NULL || state->failed || state->finished)
    {
        return 0;
    }

    size_t count = 0;

    // 2. Loop to fill the sliding window with new DATA packets
    while (state->next < state->total_chunks &&
           (state->next - state->base) < state->window)
    {
        uint32_t seq = state->next;
        size_t slot_idx = seq % WINDOW_MAX;
        packet_header *packet = &state->standby[slot_idx];

        // Format packet metadata
        packet->pack_type = DATA;
        packet->seq_num = seq;

        // Calculate slice size and copy from raw data buffer
        size_t offset = (size_t)seq * PAYLOAD_SIZE;
        size_t remaining = state->size - offset;

        packet->data_len = PAYLOAD_SIZE;
        if (remaining < PAYLOAD_SIZE)
        {
            packet->data_len = (uint16_t)remaining;
        }

        memcpy(packet->data, state->data + offset, packet->data_len);

        // Stage the packet for output transmission
        out[count] = *packet;
        count++;

        // Start the retransmission timer if this is the oldest unacknowledged packet
        if (state->base == state->next)
        {
            state->limit_ms = now_ms + state->timeout_ms;
        }

        state->next++;
    }

    // 3. Emit a FIN packet once all data is fully sent AND acknowledged
    if (state->base == state->total_chunks && state->next == state->total_chunks)
    {
        size_t slot_idx = state->next % WINDOW_MAX;
        packet_header *packet = &state->standby[slot_idx];

        packet->pack_type = FIN;
        packet->seq_num = state->next;
        packet->data_len = 0;

        // Stage the FIN packet for transmission
        out[count] = *packet;
        count++;

        // Arm the timeout timer to monitor the FIN packet delivery
        state->limit_ms = now_ms + state->timeout_ms;
        state->next++;
    }

    return count;
}

int flush(int fd, const packet_header *packets, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (send_packet(fd, &packets[i]) != 0)
        {
            return -1;
        }
        const char *packet_type = packets[i].pack_type == DATA ? "DATA" : packets[i].pack_type == FIN ? "FIN"
                                                                                                      : "UNKNOWN";
        printf("Sent %s packet: seq=%u, payload=%zu bytes\n",
               packet_type, (unsigned)packets[i].seq_num, packets[i].data_len);
    }
    return 0;
}

/**
 * Manages packet retransmissions when the sliding window timer expires.
 * Copies all unacknowledged packets to the output buffer and permanently
 * marks the sender as failed if 10 consecutive attempts pass without progress.
 *
 * @param[in,out] state  Pointer to the sender's active state machine tracking object.
 * @param[in]     now_ms The current system timestamp in milliseconds (from a monotonic clock).
 * @param[out]    out    An allocated array where unacknowledged packets will be staged for retransmission.
 * @return               The total number of packets placed into the 'out' array for resending.
 */
size_t handle_retransmission_timeout(server_state *state, int64_t now_ms, packet_header *out)
{
    // 1. Guard clauses to ensure a timeout actually occurred and data is in flight
    if (state == NULL || out == NULL || state->failed || state->finished)
    {
        return 0;
    }

    if (state->base == state->next || now_ms < state->limit_ms)
    {
        return 0;
    }

    state->num_timeouts++;
    if (state->num_timeouts >= 10)
    {
        state->failed = 1;
        return 0;
    }

    size_t count = 0;

    for (uint32_t seq = state->base; seq < state->next; seq++)
    {
        size_t slot_idx = seq % WINDOW_MAX;
        out[count] = state->standby[slot_idx];
        count++;
    }

    state->limit_ms = now_ms + state->timeout_ms;

    return count;
}

/**
 * Evaluates an incoming acknowledgment packet. On cumulative progress, slides
 * the transmission window base, resets the network failure counter, and updates
 * the retransmission deadline.
 *
 * @param[in,out] state  Pointer to the sender's active state machine tracking object.
 * @param[in]     ack    Pointer to the read-only packet data structure containing the ACK metadata.
 * @param[in]     now_ms The current system timestamp in milliseconds (from a monotonic clock).
 * @return               Returns 1 if the ACK was valid and advanced the window state; 0 otherwise.
 */
int handle_valid_ack(server_state *state, const packet_header *header, int64_t now_ms)
{
    if (state == NULL || header == NULL || state->failed || state->finished)
    {
        return 0;
    }

    if (header->pack_type != ACK || header->data_len != 0)
    {
        return 0;
    }

    if (header->seq_num <= state->base || header->seq_num > state->next)
    {
        return 0;
    }

    state->base = header->seq_num;
    state->num_timeouts = 0; // Connection is active; reset the failure safety circuit

    uint32_t final_fin_ack_target = state->total_chunks + 1;

    if (state->base == final_fin_ack_target)
    {
        state->finished = 1;
        state->limit_ms = 0;
    }
    else if (state->base == state->next)
    {
        state->limit_ms = 0;
    }
    else
    {
        state->limit_ms = now_ms + state->timeout_ms;
    }

    return 1;
}
```

### utils.c

```c

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <regex.h>
#include <errno.h>
#include <poll.h>

#ifdef TEST
extern ssize_t test_send(int socket_fd, const void *buffer, size_t length, int flags);
extern int test_clock_gettime(clockid_t clock_id, struct timespec *time_value);
extern int test_regcomp(regex_t *pattern, const char *regex, int flags);
extern FILE *test_fopen(const char *path, const char *mode);
extern int test_fseek(FILE *stream, long offset, int origin);
extern long test_ftell(FILE *stream);
extern void *test_malloc(size_t size);
extern size_t test_fread(void *buffer, size_t size, size_t count, FILE *stream);
extern int test_ferror(FILE *stream);
extern int test_feof(FILE *stream);
extern int test_fclose(FILE *stream);
#define send test_send
#define clock_gettime test_clock_gettime
#define regcomp test_regcomp
#define fopen test_fopen
#define fseek test_fseek
#define ftell test_ftell
#define malloc test_malloc
#define fread test_fread
#define ferror test_ferror
#define feof test_feof
#define fclose test_fclose
#endif

uint16_t compute_checksum(const uint8_t *data, size_t length)
{
    uint32_t sum = 0;

    while (length > 1)
    {
        sum += ((uint32_t)data[0] << 8) | data[1];
        data += 2;
        length -= 2;
    }

    if (length == 1)
    {
        sum += ((uint32_t)data[0] << 8);
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

// Returns 1 when the session name is valid; otherwise returns 0.
int session_validator(const char *session_name)
{
    regex_t pattern;
    int result;

    if (session_name == NULL)
    {
        return 0;
    }

    result = regcomp(&pattern, "^[a-z0-9-]{1,32}$", REG_EXTENDED | REG_NOSUB);
    if (result != 0)
    {
        return 0;
    }

    result = regexec(&pattern, session_name, 0, NULL, 0) == 0;
    regfree(&pattern);

    return result;
}

FILE_METADATA *read_file(const char *path)
{
    FILE *inputFile;
    FILE_METADATA *file = NULL;

    if ((inputFile = fopen(path, "rb")) == NULL)
    {
        perror("fopen");
        return NULL;
    }

    fseek(inputFile, 0, SEEK_END);

    long file_size = ftell(inputFile);
    if (file_size < 0 || file_size > MAX_SIZE_OF_FILE)
    {
        perror("Error getting file position");
        fclose(inputFile);
        return NULL;
    }

    fseek(inputFile, 0, SEEK_SET);

    file = malloc(sizeof(*file));
    if (file == NULL)
    {
        perror("Memory allocation failed");
        fclose(inputFile);
        return NULL;
    }

    file->data = malloc((size_t)file_size);
    if (file->data == NULL && file_size != 0)
    {
        perror("Memory allocation failed");
        free(file);
        fclose(inputFile);
        return NULL;
    }

    size_t bytes_read = fread(file->data, 1, (size_t)file_size, inputFile);

    if (bytes_read < (size_t)file_size)
    {
        if (ferror(inputFile))
        {
            perror("Error reading file");
        }
        else if (feof(inputFile))
        {
            printf("Warning: Hit End-of-File early. Read %zu of %ld bytes.\n", bytes_read, file_size);
        }
    }

    fclose(inputFile);

    file->size = bytes_read;
    return file;
}

/**
 * Returns the current system time in milliseconds using a monotonic clock.
 * Guaranteed never to move backwards, making it ideal for timeouts and intervals.
 *
 * @return Current timestamp in milliseconds, or -1 on error.
 */
int64_t get_time_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return -1;
    }

    const int64_t MS_PER_SEC = 1000;
    const int64_t NS_PER_MS = 1000000;

    return ((int64_t)ts.tv_sec * MS_PER_SEC) + (ts.tv_nsec / NS_PER_MS);
}
/**
 * Send the package
 */
int send_packet(int sock_fd, const packet_header *incoming_packet)
{
    if (incoming_packet == NULL || incoming_packet->pack_type < DATA || incoming_packet->pack_type > FIN ||
        incoming_packet->data_len > PAYLOAD_SIZE ||
        (incoming_packet->pack_type != DATA && incoming_packet->data_len != 0))
    {
        return -1;
    }

    uint8_t wire_packet[HEADER_SIZE + PAYLOAD_SIZE] = {0};
    size_t totalBytes = HEADER_SIZE + incoming_packet->data_len;

    wire_packet[0] = (uint8_t)incoming_packet->pack_type;

    wire_packet[4] = (uint8_t)(incoming_packet->seq_num >> 24);
    wire_packet[5] = (uint8_t)(incoming_packet->seq_num >> 16);
    wire_packet[6] = (uint8_t)(incoming_packet->seq_num >> 8);
    wire_packet[7] = (uint8_t)incoming_packet->seq_num;

    wire_packet[8] = (uint8_t)(incoming_packet->data_len >> 8);
    wire_packet[9] = (uint8_t)incoming_packet->data_len;
    if (incoming_packet->data_len > 0)
    {
        memcpy(wire_packet + HEADER_SIZE, incoming_packet->data, incoming_packet->data_len);
    }

    uint16_t checksum = compute_checksum(wire_packet, totalBytes);
    wire_packet[2] = (uint8_t)(checksum >> 8);
    wire_packet[3] = (uint8_t)checksum;
    ssize_t bytes_sent = send(sock_fd, wire_packet, totalBytes, 0);
    if (bytes_sent < 0)
    {
        perror("Failed to send packet");
        return -1;
    }
    if ((size_t)bytes_sent != totalBytes)
    {
        return -1;
    }
    return 0;
}
int parse_incoming_packet(const uint8_t *packetPayload, size_t size, packet_header *outputHeader)
{
    if (packetPayload == NULL || outputHeader == NULL || size < HEADER_SIZE)
    {
        return -1;
    }

    uint8_t type = packetPayload[0];
    uint8_t reserved = packetPayload[1];

    if (type > FIN || reserved != 0)
    {
        return -1;
    }

    uint16_t data_length = (uint16_t)(((uint16_t)packetPayload[8] << 8) | packetPayload[9]);

    if (data_length > PAYLOAD_SIZE)
    {
        return -1;
    }
    if (HEADER_SIZE + data_length != size)
    {
        return -1;
    }
    if (type != DATA && data_length != 0)
    {
        return -1; // Only DATA packets are allowed to carry a payload
    }

    if (compute_checksum(packetPayload, size) != 0)
    {
        return -1;
    }

    packet_header parsed_data = {
        .pack_type = type,
        .data_len = data_length,
        .seq_num = ((uint32_t)packetPayload[4] << 24) |
                   ((uint32_t)packetPayload[5] << 16) |
                   ((uint32_t)packetPayload[6] << 8) |
                   (uint32_t)packetPayload[7]};

    if (data_length > 0)
    {
        memcpy(parsed_data.data, packetPayload + HEADER_SIZE, data_length);
    }

    *outputHeader = parsed_data;
    return 0;
}

/**
 * Evaluates an incoming server handshake response and classifies it.
 *
 * @param[in]  reply   Pointer to the raw character array containing the server response.
 * @param[in]  length  The total byte length of the received response string.
 * @return             The classified handshake status (REG_SUCCESS, REG_FAILURE, or REG_MALFORMED).
 */
registration_status evaluate_registration_response(const char *reply, size_t length)
{
    // 1. Reject basic null references immediately
    if (reply == NULL)
    {
        return REG_MALFORMED;
    }

    if (length == 2 && memcmp(reply, "OK", 2) == 0)
    {
        return REG_SUCCESS;
    }

    const size_t ERR_PREFIX_LEN = 4; // Length of "ERR "

    if (length >= (ERR_PREFIX_LEN + 1) && memcmp(reply, "ERR ", ERR_PREFIX_LEN) == 0)
    {
        return REG_FAILURE;
    }

    return REG_MALFORMED;
}
```

### utils.h

```c


#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include "lab.h"

#define STREQU(a, b) (strcmp(a, b) == 0)

uint16_t compute_checksum(const uint8_t *data, size_t length);

int session_validator(const char *session);

int send_packet(int sock_fd, const packet_header *incoming_packet);
int parse_incoming_packet(const uint8_t *packetPayload, size_t size, packet_header *outputHeader);

int64_t get_time_ms(void);

typedef struct
{
    uint8_t *data;
    size_t size;
} FILE_METADATA;

FILE_METADATA *read_file(const char *path);

typedef enum
{
    REG_SUCCESS,
    REG_FAILURE,
    REG_MALFORMED
} registration_status;

registration_status evaluate_registration_response(const char *reply, size_t length);

#endif // UTILS_H

```

## Tests Files
### lab-test.c

```c

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <regex.h>
#include "harness/unity.h"
#include "../src/lab.h"
#include "../src/utils.h"

static int mock_io_enabled;
static int mock_send_failure;
static int mock_send_short;
static int mock_send_fail_after;
static int mock_recv_failure;
static int mock_recv_fail_count;
static int mock_recv_errno = EIO;
static int mock_registration_reply_enabled;
static char mock_registration_reply[250];
static size_t mock_registration_reply_length;
static int mock_setsockopt_failure;
static int mock_fopen_failure;
static int mock_remaining_timeout_enabled;
static int mock_remaining_timeout_value;
static int mock_remaining_timeout_errno;
static int mock_file_io_enabled;
static long mock_ftell_value;
static const char *mock_file_contents;
static size_t mock_fread_bytes;
static int mock_ferror_value;
static int mock_feof_value;
static int mock_regcomp_failure;
static int mock_malloc_fail_at;
static int mock_malloc_calls;
static int mock_file_token;
static int mock_poll_result = 1;
static int mock_poll_interrupt_count;
static int mock_poll_errno = EIO;
static int mock_socket_fail_count;
static int mock_connect_fail_count;
static int mock_connect_enabled;
static int mock_server_network_enabled;
static int mock_getopt_enabled;
static int mock_getopt_value;
static int mock_clock_enabled;
static int mock_clock_failure;
static int mock_clock_calls;
static int mock_clock_timeout;
static int mock_clock_step_enabled;
static int mock_clock_step_ms;
static uint8_t mock_receive_packets[4][HEADER_SIZE + PAYLOAD_SIZE];
static size_t mock_receive_sizes[4];
static size_t mock_receive_count;
static size_t mock_receive_index;
static uint8_t mock_sent_packets[16][HEADER_SIZE + PAYLOAD_SIZE];
static size_t mock_sent_sizes[16];
static size_t mock_sent_count;
static struct addrinfo mock_server_addrinfo;
static struct sockaddr_in mock_server_sockaddr;

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
    if (mock_send_short)
    {
      return (ssize_t)(length - 1);
    }
    if (mock_send_fail_after > 0 && --mock_send_fail_after == 0)
    {
      errno = EIO;
      return -1;
    }
    if (mock_sent_count >= sizeof(mock_sent_packets) / sizeof(mock_sent_packets[0]) ||
        length > sizeof(mock_sent_packets[0]))
    {
      errno = EMSGSIZE;
      return -1;
    }
    memcpy(mock_sent_packets[mock_sent_count], buffer, length);
    mock_sent_sizes[mock_sent_count] = length;
    mock_sent_count++;
    return (ssize_t)length;
  }
  return send(socket_fd, buffer, length, flags);
}

ssize_t test_recv(int socket_fd, void *buffer, size_t length, int flags)
{
  if (mock_io_enabled)
  {
    if (mock_recv_fail_count > 0)
    {
      mock_recv_fail_count--;
      errno = mock_recv_errno;
      return -1;
    }
    if (mock_recv_failure)
    {
      errno = mock_recv_errno;
      return -1;
    }
    if (mock_registration_reply_enabled)
    {
      size_t reply_length = mock_registration_reply_length;
      if (reply_length > length)
      {
        reply_length = length;
      }
      memcpy(buffer, mock_registration_reply, reply_length);
      mock_registration_reply_enabled = 0;
      return (ssize_t)reply_length;
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
    if (mock_poll_interrupt_count > 0)
    {
      mock_poll_interrupt_count--;
      errno = EINTR;
      return -1;
    }
    if (mock_poll_result < 0)
    {
      errno = mock_poll_errno;
    }
    return mock_poll_result;
  }
  return poll(fds, count, timeout);
}

int test_socket(int domain, int type, int protocol)
{
  if (mock_socket_fail_count > 0)
  {
    mock_socket_fail_count--;
    errno = EMFILE;
    return -1;
  }
  return socket(domain, type, protocol);
}

int test_connect(int socket_fd, const struct sockaddr *addr, socklen_t addr_len)
{
  if (mock_connect_enabled)
  {
    if (mock_connect_fail_count > 0)
    {
      mock_connect_fail_count--;
      errno = ECONNREFUSED;
      return -1;
    }
    return 0;
  }
  if (mock_connect_fail_count > 0)
  {
    mock_connect_fail_count--;
    errno = ECONNREFUSED;
    return -1;
  }
  return connect(socket_fd, addr, addr_len);
}

int test_getaddrinfo(const char *node, const char *service,
                     const struct addrinfo *hints, struct addrinfo **result)
{
  if (!mock_server_network_enabled)
  {
    return getaddrinfo(node, service, hints, result);
  }
  if (strcmp(node, "invalid host name") == 0)
  {
    return EAI_NONAME;
  }
  memset(&mock_server_addrinfo, 0, sizeof(mock_server_addrinfo));
  memset(&mock_server_sockaddr, 0, sizeof(mock_server_sockaddr));
  mock_server_sockaddr.sin_family = AF_INET;
  mock_server_sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  mock_server_sockaddr.sin_port = htons((uint16_t)strtoul(service, NULL, 10));
  mock_server_addrinfo.ai_family = AF_INET;
  mock_server_addrinfo.ai_socktype = SOCK_DGRAM;
  mock_server_addrinfo.ai_protocol = IPPROTO_UDP;
  mock_server_addrinfo.ai_addr = (struct sockaddr *)&mock_server_sockaddr;
  mock_server_addrinfo.ai_addrlen = sizeof(mock_server_sockaddr);
  *result = &mock_server_addrinfo;
  return 0;
}

void test_freeaddrinfo(struct addrinfo *result)
{
  if (!mock_server_network_enabled)
  {
    freeaddrinfo(result);
  }
}

int test_getopt(int argc, char *const argv[], const char *options)
{
  if (mock_getopt_enabled)
  {
    mock_getopt_enabled = 0;
    return mock_getopt_value;
  }
  return getopt(argc, argv, options);
}

int test_setsockopt(int socket_fd, int level, int option_name,
                    const void *option_value, socklen_t option_length)
{
  (void)socket_fd;
  (void)level;
  (void)option_name;
  (void)option_value;
  (void)option_length;
  if (mock_setsockopt_failure)
  {
    errno = EIO;
    return -1;
  }
  return 0;
}

FILE *test_fopen(const char *path, const char *mode)
{
  if (mock_file_io_enabled)
  {
    (void)path;
    (void)mode;
    return (FILE *)&mock_file_token;
  }
  if (mock_fopen_failure)
  {
    errno = EACCES;
    return NULL;
  }
  return fopen(path, mode);
}

int test_regcomp(regex_t *pattern, const char *regex, int flags)
{
  if (mock_regcomp_failure)
  {
    return REG_ESPACE;
  }
  return regcomp(pattern, regex, flags);
}

int test_fseek(FILE *stream, long offset, int origin)
{
  if (mock_file_io_enabled)
  {
    (void)stream;
    (void)offset;
    (void)origin;
    return 0;
  }
  return fseek(stream, offset, origin);
}

long test_ftell(FILE *stream)
{
  if (mock_file_io_enabled)
  {
    (void)stream;
    return mock_ftell_value;
  }
  return ftell(stream);
}

void *test_malloc(size_t size)
{
  mock_malloc_calls++;
  if (mock_malloc_fail_at > 0 && mock_malloc_calls == mock_malloc_fail_at)
  {
    return NULL;
  }
  return malloc(size);
}

size_t test_fread(void *buffer, size_t size, size_t count, FILE *stream)
{
  if (mock_file_io_enabled)
  {
    (void)stream;
    size_t bytes_to_copy = mock_fread_bytes;
    size_t requested_bytes = size * count;
    if (bytes_to_copy > requested_bytes)
    {
      bytes_to_copy = requested_bytes;
    }
    if (bytes_to_copy > 0)
    {
      memcpy(buffer, mock_file_contents, bytes_to_copy);
    }
    return size == 0 ? 0 : bytes_to_copy / size;
  }
  return fread(buffer, size, count, stream);
}

int test_ferror(FILE *stream)
{
  if (mock_file_io_enabled)
  {
    (void)stream;
    return mock_ferror_value;
  }
  return ferror(stream);
}

int test_feof(FILE *stream)
{
  if (mock_file_io_enabled)
  {
    (void)stream;
    return mock_feof_value;
  }
  return feof(stream);
}

int test_fclose(FILE *stream)
{
  if (mock_file_io_enabled)
  {
    (void)stream;
    return 0;
  }
  return fclose(stream);
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
    if (mock_clock_step_enabled)
    {
      int64_t milliseconds = 1000 + (int64_t)(mock_clock_calls - 1) * mock_clock_step_ms;
      time_value->tv_sec = (time_t)(milliseconds / 1000);
      time_value->tv_nsec = (long)(milliseconds % 1000) * 1000000L;
      return 0;
    }
    if (mock_clock_timeout)
    {
      time_value->tv_sec = mock_clock_calls <= 2 ? 1 : 31;
      time_value->tv_nsec = 0;
      return 0;
    }
    time_value->tv_sec = mock_clock_calls <= 6 ? 1 : 3;
    time_value->tv_nsec = 0;
    return 0;
  }
  return clock_gettime(clock_id, time_value);
}

int test_get_remaining_timeout_ms(const client_state *state, int64_t now)
{
  if (mock_remaining_timeout_enabled)
  {
    if (mock_remaining_timeout_errno != 0)
    {
      errno = mock_remaining_timeout_errno;
    }
    return mock_remaining_timeout_value;
  }
  return get_remaining_timeout_ms(state, now);
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
  mock_io_enabled = 0;
  mock_send_failure = 0;
  mock_send_short = 0;
  mock_send_fail_after = 0;
  mock_recv_failure = 0;
  mock_recv_fail_count = 0;
  mock_recv_errno = EIO;
  mock_registration_reply_enabled = 0;
  mock_registration_reply_length = 0;
  mock_setsockopt_failure = 0;
  mock_fopen_failure = 0;
  mock_remaining_timeout_enabled = 0;
  mock_remaining_timeout_value = 0;
  mock_remaining_timeout_errno = 0;
  mock_file_io_enabled = 0;
  mock_ftell_value = 0;
  mock_file_contents = NULL;
  mock_fread_bytes = 0;
  mock_ferror_value = 0;
  mock_feof_value = 0;
  mock_regcomp_failure = 0;
  mock_malloc_fail_at = 0;
  mock_malloc_calls = 0;
  mock_poll_result = 1;
  mock_poll_interrupt_count = 0;
  mock_poll_errno = EIO;
  mock_socket_fail_count = 0;
  mock_connect_fail_count = 0;
  mock_connect_enabled = 0;
  mock_server_network_enabled = 0;
  mock_getopt_enabled = 0;
  mock_getopt_value = 0;
  mock_clock_enabled = 0;
  mock_clock_failure = 0;
  mock_clock_timeout = 0;
  mock_clock_step_enabled = 0;
  mock_clock_step_ms = 0;
  mock_clock_calls = 0;
  mock_receive_count = 0;
  mock_receive_index = 0;
  mock_sent_count = 0;
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

void test_registration_response_classifier(void)
{
  TEST_ASSERT_EQUAL_INT(REG_SUCCESS, evaluate_registration_response("OK", 2));
  TEST_ASSERT_EQUAL_INT(REG_FAILURE, evaluate_registration_response("ERR unknown", 11));
  TEST_ASSERT_EQUAL_INT(REG_MALFORMED, evaluate_registration_response(NULL, 2));
  TEST_ASSERT_EQUAL_INT(REG_MALFORMED, evaluate_registration_response("OK\n", 3));
  TEST_ASSERT_EQUAL_INT(REG_MALFORMED, evaluate_registration_response("ERR ", 4));
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

  packet[1] = 0;
  packet[8] = 0x04;
  packet[9] = 0x01;
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(packet, sizeof(packet), &output));
  packet[8] = 0;
  packet[9] = 4;
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(packet, sizeof(packet), &output));
  packet[9] = 1;
  packet[0] = FIN;
  TEST_ASSERT_EQUAL_INT(-1, parse_incoming_packet(packet, HEADER_SIZE + 1, &output));
}

void test_packet_sending(void)
{
  packet_header packet = {.pack_type = DATA, .seq_num = 4, .data_len = 4, .data = "data"};
  packet_header decoded = {0};

  mock_sent_count = 0;
  mock_io_enabled = 1;
  TEST_ASSERT_EQUAL_INT(0, send_packet(42, &packet));
  mock_io_enabled = 0;
  TEST_ASSERT_EQUAL_UINT(1, mock_sent_count);
  ssize_t received_bytes = (ssize_t)mock_sent_sizes[0];
  TEST_ASSERT_EQUAL_INT(HEADER_SIZE + 4, received_bytes);
  TEST_ASSERT_EQUAL_INT(DATA, mock_sent_packets[0][0]);
  TEST_ASSERT_EQUAL_INT(4, mock_sent_packets[0][7]);
  TEST_ASSERT_EQUAL_MEMORY("data", mock_sent_packets[0] + HEADER_SIZE, 4);
  TEST_ASSERT_EQUAL_INT(0, parse_incoming_packet(mock_sent_packets[0], (size_t)received_bytes, &decoded));
  TEST_ASSERT_EQUAL_UINT32(4, decoded.seq_num);
  TEST_ASSERT_EQUAL_MEMORY("data", decoded.data, 4);

  TEST_ASSERT_EQUAL_INT(-1, send_packet(42, NULL));
  packet.data_len = PAYLOAD_SIZE + 1;
  TEST_ASSERT_EQUAL_INT(-1, send_packet(42, &packet));
  packet.data_len = 1;
  packet.pack_type = FIN;
  TEST_ASSERT_EQUAL_INT(-1, send_packet(42, &packet));

  packet.pack_type = DATA;
  packet.data_len = 4;
  mock_send_short = 1;
  mock_io_enabled = 1;
  TEST_ASSERT_EQUAL_INT(-1, send_packet(42, &packet));
  mock_io_enabled = 0;
  mock_send_short = 0;
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

void test_read_file_error_paths(void)
{
  FILE_METADATA *metadata;

  mock_regcomp_failure = 1;
  TEST_ASSERT_FALSE(session_validator("valid"));
  mock_regcomp_failure = 0;

  mock_file_io_enabled = 1;
  mock_file_contents = "abc";
  mock_fread_bytes = 3;
  mock_ftell_value = -1;
  TEST_ASSERT_NULL(read_file("mocked"));
  mock_ftell_value = (long)MAX_SIZE_OF_FILE + 1;
  TEST_ASSERT_NULL(read_file("mocked"));

  mock_ftell_value = 3;
  mock_malloc_fail_at = 1;
  mock_malloc_calls = 0;
  TEST_ASSERT_NULL(read_file("mocked"));
  mock_malloc_fail_at = 2;
  mock_malloc_calls = 0;
  TEST_ASSERT_NULL(read_file("mocked"));
  mock_malloc_fail_at = 0;

  mock_ftell_value = 10;
  mock_ferror_value = 1;
  metadata = read_file("mocked");
  TEST_ASSERT_NOT_NULL(metadata);
  TEST_ASSERT_EQUAL_UINT(3, metadata->size);
  TEST_ASSERT_EQUAL_MEMORY("abc", metadata->data, 3);
  free(metadata->data);
  free(metadata);

  mock_ferror_value = 0;
  mock_feof_value = 1;
  metadata = read_file("mocked");
  TEST_ASSERT_NOT_NULL(metadata);
  TEST_ASSERT_EQUAL_UINT(3, metadata->size);
  free(metadata->data);
  free(metadata);

  mock_ftell_value = 0;
  mock_fread_bytes = 0;
  metadata = read_file("mocked");
  TEST_ASSERT_NOT_NULL(metadata);
  TEST_ASSERT_EQUAL_UINT(0, metadata->size);
  free(metadata->data);
  free(metadata);
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
  mock_getopt_value = 'z';
  mock_getopt_enabled = 1;
  TEST_ASSERT_NULL(parse_ser_opt(3, server_argv));
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

  mock_socket_fail_count = 1;
  int retried_client_fd = init_client(&client);
  mock_socket_fail_count = 0;
  TEST_ASSERT_TRUE(retried_client_fd >= 0);
  close(retried_client_fd);

  mock_connect_fail_count = 1;
  int reconnected_client_fd = init_client(&client);
  mock_connect_fail_count = 0;
  TEST_ASSERT_TRUE(reconnected_client_fd >= 0);
  close(reconnected_client_fd);

  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  TEST_ASSERT_EQUAL_INT((int)strlen(reply), (int)send(sockets[1], reply, strlen(reply), 0));
  TEST_ASSERT_EQUAL_INT(0, register_client(sockets[0], &client));
  close(sockets[0]);
  close(sockets[1]);

  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  TEST_ASSERT_EQUAL_INT((int)strlen(reply), (int)send(sockets[1], reply, strlen(reply), 0));
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

void test_registration_response_handling(void)
{
  CLIENT_ARGUMENT client = {.session = "client"};
  SERVER_ARGUMENT server = {.session = "server"};

  mock_io_enabled = 1;
  memcpy(mock_registration_reply, "OK", 2);
  mock_registration_reply_length = 2;
  mock_registration_reply_enabled = 1;
  TEST_ASSERT_EQUAL_INT(0, register_client(42, &client));

  memcpy(mock_registration_reply, "ERR denied", 10);
  mock_registration_reply_length = 10;
  mock_registration_reply_enabled = 1;
  TEST_ASSERT_EQUAL_INT(2, register_client(42, &client));

  memcpy(mock_registration_reply, "NO", 2);
  mock_registration_reply_length = 2;
  mock_registration_reply_enabled = 1;
  TEST_ASSERT_EQUAL_INT(2, register_client(42, &client));

  memcpy(mock_registration_reply, "OK", 2);
  mock_registration_reply_length = 2;
  mock_registration_reply_enabled = 1;
  TEST_ASSERT_EQUAL_INT(0, register_server(42, &server));

  memcpy(mock_registration_reply, "ERR denied", 10);
  mock_registration_reply_length = 10;
  mock_registration_reply_enabled = 1;
  TEST_ASSERT_EQUAL_INT(2, register_server(42, &server));

  memcpy(mock_registration_reply, "NO", 2);
  mock_registration_reply_length = 2;
  mock_registration_reply_enabled = 1;
  TEST_ASSERT_EQUAL_INT(2, register_server(42, &server));

  mock_io_enabled = 0;
}

void test_server_socket_initialization_paths(void)
{
  SERVER_ARGUMENT server = {.relay = "relay.test", .port = RELAY_PORT};

  mock_server_network_enabled = 1;
  server.relay = "invalid host name";
  TEST_ASSERT_EQUAL_INT(1, init_server(&server));
  server.relay = "relay.test";

  mock_socket_fail_count = 1;
  TEST_ASSERT_EQUAL_INT(-1, init_server(&server));

  mock_connect_enabled = 1;
  mock_connect_fail_count = 1;
  TEST_ASSERT_EQUAL_INT(-1, init_server(&server));

  mock_connect_fail_count = 0;
  int socket_fd = init_server(&server);
  TEST_ASSERT_TRUE(socket_fd >= 0);
  close(socket_fd);
}

void test_timeout_and_consume(void)
{
  client_state state = {.expected = 3, .finished = 0, .last_valid_ms = 1000};
  packet_header packet = {.pack_type = DATA, .seq_num = 3, .data_len = 0};
  FILE *file = tmpfile();
  int sockets[2];
  uint8_t ack[HEADER_SIZE + PAYLOAD_SIZE];

  TEST_ASSERT_EQUAL_INT(30000, get_remaining_timeout_ms(&state, 1000));
  TEST_ASSERT_EQUAL_INT(0, get_remaining_timeout_ms(&state, 31000));
  TEST_ASSERT_EQUAL_INT(0, get_remaining_timeout_ms(NULL, 0));
  TEST_ASSERT_NOT_NULL(file);
  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  TEST_ASSERT_EQUAL_INT(0, consume(sockets[0], &state, &packet, file));
  TEST_ASSERT_EQUAL_UINT32(4, state.expected);
  TEST_ASSERT_EQUAL_INT(HEADER_SIZE, recv(sockets[1], ack, sizeof(ack), 0));
  TEST_ASSERT_EQUAL_INT(ACK, ack[0]);
  TEST_ASSERT_EQUAL_UINT8(4, ack[7]);
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

  server_state partial_state = {.base = 0, .next = 3, .total_chunks = 5, .timeout_ms = 25};
  packet_header partial_ack = {.pack_type = ACK, .seq_num = 1, .data_len = 0};
  TEST_ASSERT_EQUAL_INT(1, handle_valid_ack(&partial_state, &partial_ack, 200));
  TEST_ASSERT_EQUAL_UINT32(1, partial_state.base);
  TEST_ASSERT_EQUAL_INT64(225, partial_state.limit_ms);
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
  TEST_ASSERT_EQUAL_INT(-1, send_ack(sockets[0], packet_header_value.seq_num));
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
  uint8_t ack[HEADER_SIZE + PAYLOAD_SIZE];

  TEST_ASSERT_NOT_NULL(file);
  TEST_ASSERT_EQUAL_INT(0, make_nonblocking_socketpair(sockets));
  TEST_ASSERT_EQUAL_INT(-1, consume(sockets[0], NULL, &data, file));
  TEST_ASSERT_EQUAL_INT(-1, consume(sockets[0], &state, NULL, file));
  TEST_ASSERT_EQUAL_INT(-1, consume(sockets[0], &state, &data, NULL));
  data.data_len = PAYLOAD_SIZE + 1;
  TEST_ASSERT_EQUAL_INT(-1, consume(sockets[0], &state, &data, file));
  data.data_len = 3;
  TEST_ASSERT_EQUAL_INT(0, consume(sockets[0], &state, &data, file));
  TEST_ASSERT_EQUAL_INT(3, (int)ftell(file));
  TEST_ASSERT_EQUAL_INT(HEADER_SIZE, recv(sockets[1], ack, sizeof(ack), 0));
  TEST_ASSERT_EQUAL_UINT8(2, ack[7]);
  TEST_ASSERT_EQUAL_INT(0, consume(sockets[0], &state, &duplicate, file));
  TEST_ASSERT_EQUAL_INT(HEADER_SIZE, recv(sockets[1], ack, sizeof(ack), 0));
  TEST_ASSERT_EQUAL_UINT8(2, ack[7]);
  TEST_ASSERT_EQUAL_INT(0, consume(sockets[0], &state, &fin, file));
  TEST_ASSERT_EQUAL_INT(HEADER_SIZE, recv(sockets[1], ack, sizeof(ack), 0));
  TEST_ASSERT_EQUAL_UINT8(3, ack[7]);
  TEST_ASSERT_TRUE(state.finished);
  TEST_ASSERT_EQUAL_INT(0, fseek(file, 0, SEEK_SET));
  TEST_ASSERT_EQUAL_INT(0, consume(sockets[0], &state, &fin, NULL));
  TEST_ASSERT_EQUAL_INT(HEADER_SIZE, recv(sockets[1], ack, sizeof(ack), 0));
  TEST_ASSERT_EQUAL_UINT8(3, ack[7]);
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

void test_process_open_failure(void)
{
  CLIENT_ARGUMENT client = {.file_name = "mocked-file"};

  mock_fopen_failure = 1;
  TEST_ASSERT_EQUAL_INT(2, process(42, &client));
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
  mock_sent_count = 0;
  queue_wire_packet(DATA, 0, NULL, 0);
  mock_receive_packets[0][1] = 1;
  queue_wire_packet(ACK, 1, NULL, 0);
  queue_wire_packet(ACK, 2, NULL, 0);
  mock_io_enabled = 1;
  mock_poll_interrupt_count = 1;
  mock_recv_fail_count = 1;
  mock_recv_errno = EINTR;
  TEST_ASSERT_EQUAL_INT(0, publish(42, &server));
  mock_io_enabled = 0;
  TEST_ASSERT_EQUAL_UINT(2, mock_sent_count);
  TEST_ASSERT_EQUAL_UINT(HEADER_SIZE + strlen(contents), mock_sent_sizes[0]);
  TEST_ASSERT_EQUAL_INT(DATA, mock_sent_packets[0][0]);
  TEST_ASSERT_EQUAL_MEMORY(contents, mock_sent_packets[0] + HEADER_SIZE, strlen(contents));
  TEST_ASSERT_EQUAL_UINT(HEADER_SIZE, mock_sent_sizes[1]);
  TEST_ASSERT_EQUAL_INT(FIN, mock_sent_packets[1][0]);
  unlink(path);
}

void test_publish_error_and_timeout_paths(void)
{
  char path[] = "/tmp/cs425-publish-paths-XXXXXX";
  const char contents[] = "x";
  int file_fd = mkstemp(path);
  SERVER_ARGUMENT server = {.file_name = path, .window = 1, .timeout_ms = 1000};

  TEST_ASSERT_TRUE(file_fd >= 0);
  TEST_ASSERT_EQUAL_INT(1, (int)write(file_fd, contents, sizeof(contents) - 1));
  close(file_fd);
  mock_io_enabled = 1;

  mock_fopen_failure = 1;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
  mock_fopen_failure = 0;

  mock_send_failure = 1;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
  mock_send_failure = 0;

  server.window = 0;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
  server.window = (int)WINDOW_MAX + 1;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
  server.window = 1;
  server.timeout_ms = 0;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));

  server.timeout_ms = 1000;
  mock_malloc_fail_at = 3;
  mock_malloc_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
  mock_malloc_fail_at = 0;

  mock_receive_count = 0;
  mock_receive_index = 0;
  mock_sent_count = 0;
  mock_send_fail_after = 2;
  queue_wire_packet(ACK, 1, NULL, 0);
  mock_clock_enabled = 1;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
  mock_clock_enabled = 0;
  mock_send_fail_after = 0;

  server.timeout_ms = 1;
  mock_sent_count = 0;
  mock_send_fail_after = 2;
  mock_clock_enabled = 1;
  mock_clock_step_enabled = 1;
  mock_clock_step_ms = 10;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
  mock_send_fail_after = 0;

  server.timeout_ms = 100;
  mock_sent_count = 0;
  mock_poll_result = 0;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, publish(42, &server));
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

void test_process_idle_timeout(void)
{
  char path[] = "/tmp/cs425-timeout-XXXXXX";
  CLIENT_ARGUMENT client = {.file_name = path};
  int file_fd = mkstemp(path);

  TEST_ASSERT_TRUE(file_fd >= 0);
  close(file_fd);
  mock_io_enabled = 1;
  mock_recv_failure = 1;
  mock_recv_errno = EAGAIN;
  mock_clock_enabled = 1;
  mock_clock_timeout = 1;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, process(42, &client));
  unlink(path);
}

void test_process_uncovered_timeout_and_packet_paths(void)
{
  char path[] = "/tmp/cs425-client-branches-XXXXXX";
  CLIENT_ARGUMENT client = {.file_name = path};
  int file_fd = mkstemp(path);

  TEST_ASSERT_TRUE(file_fd >= 0);
  close(file_fd);
  mock_io_enabled = 1;
  mock_clock_enabled = 1;
  mock_clock_timeout = 1;
  mock_remaining_timeout_enabled = 1;
  mock_remaining_timeout_value = -1;
  mock_remaining_timeout_errno = EINTR;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, process(42, &client));

  mock_remaining_timeout_errno = EIO;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, process(42, &client));

  mock_remaining_timeout_value = 0;
  mock_remaining_timeout_errno = 0;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, process(42, &client));

  mock_remaining_timeout_enabled = 0;
  mock_recv_failure = 1;
  mock_recv_errno = EINTR;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, process(42, &client));

  mock_recv_errno = EAGAIN;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, process(42, &client));

  mock_recv_failure = 0;
  mock_receive_count = 0;
  mock_receive_index = 0;
  queue_wire_packet(DATA, 0, NULL, 0);
  mock_receive_packets[0][1] = 1;
  mock_clock_calls = 0;
  TEST_ASSERT_EQUAL_INT(2, process(42, &client));
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

  mock_io_enabled = 1;
  mock_setsockopt_failure = 1;
  TEST_ASSERT_EQUAL_INT(-1, register_client(42, &client));
  TEST_ASSERT_EQUAL_INT(-1, register_server(42, &server));
  mock_setsockopt_failure = 0;
  mock_recv_failure = 1;
  mock_recv_errno = EIO;
  TEST_ASSERT_EQUAL_INT(-1, register_client(42, &client));
  mock_recv_errno = EAGAIN;
  TEST_ASSERT_EQUAL_INT(-1, register_client(42, &client));

  mock_recv_errno = EIO;
  mock_sent_count = 0;
  TEST_ASSERT_EQUAL_INT(-1, register_server(42, &server));
  mock_recv_errno = EAGAIN;
  mock_sent_count = 0;
  TEST_ASSERT_EQUAL_INT(-1, register_server(42, &server));
  mock_io_enabled = 0;
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
  RUN_TEST(test_registration_response_classifier);
  RUN_TEST(test_parse_incoming_packet);
  RUN_TEST(test_packet_sending);
  RUN_TEST(test_read_file_and_get_time);
  RUN_TEST(test_read_file_error_paths);
  RUN_TEST(test_parse_options_invalid);
  RUN_TEST(test_network_initialization_and_registration);
  RUN_TEST(test_registration_response_handling);
  RUN_TEST(test_server_socket_initialization_paths);
  RUN_TEST(test_timeout_and_consume);
  RUN_TEST(test_sender_state_machine);
  RUN_TEST(test_protocol_error_branches);
  RUN_TEST(test_consume_all_packet_cases);
  RUN_TEST(test_state_guards_and_timeout_paths);
  RUN_TEST(test_retransmission_and_flush);
  RUN_TEST(test_process_and_publish_invalid_inputs);
  RUN_TEST(test_process_open_failure);
  RUN_TEST(test_publish_success);
  RUN_TEST(test_publish_error_and_timeout_paths);
  RUN_TEST(test_process_success);
  RUN_TEST(test_process_idle_timeout);
  RUN_TEST(test_process_uncovered_timeout_and_packet_paths);
  RUN_TEST(test_mocked_io_failures);
  RUN_TEST(test_mocked_receive_and_publish_failures);
  RUN_TEST(test_publish_poll_and_clock_failures);
  return UNITY_END();
}

```

## Scripts Files
Report generated on 09/29/2026 at 03:25:03


---

## End of Report

SHA-256 Hash of the report: 08cbe006deff40f8cdfe88acb7b4489efc7de65be32a0f7d88aba4bb41490b88

Do not edit the generated report. Any changes will be reported as academic dishonesty

---
## GitHub Info
- GitHub repo name: Duahau1/cs425-p2
- The repository visibility is public.
- The workflow was triggered by Duahau1
