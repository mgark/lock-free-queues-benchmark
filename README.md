# lock-free-queues-benchmark

## Requirements  

Need to have three major categories of tests
 - plain throughput tests where we just produce and consume message as fast as possible and than simply 
 - ping-pong tests to measure round-trip times for messages
 - tests which simulate real-world behavior closer, by not just constantly pushing messages in the tight loop, but doing that randomly and also performing other tasks to make CPU instruction's and data cache dirty
  

Another important feature must be ability to run either test with difference parameters, including the following:
  - lock-free queue type
  - number of producer / consumer threads
  - message type
  - how data is generated for each message
  - provide the list of cores threads should be pinned to

The above requirements necessitate the creation of the following utilities:
  - benchmark framework - should be kept as simple as possible
  - TSC clock for CPUs which support it
  - pinning threads to cores
  - translating from clock cycles to nanoseconds  
**For now, we can assume only Linux based system and CPUs with invariant TSC are supported**