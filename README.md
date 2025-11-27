# My Redis

This project implements a Redis-like client and server in C/C++. Redis is an in-memory key-value store known for its speed and low latency. This project incorporates many of the optimizations present in the real Redis implementation (intrusive data structures, thread pools, progressive hash table resizing, etc.) to achieve these characteristics.

## Set-up

1. Build the client and server by running `make`
2. Start the server: `./server`
3. Send commands to the server with the client: `./client [command]`

## Commands

`get <key>` - Gets the entry for _key_.

Example:
```
client> get name
(nil)
client> set name tyler
(string) "OK"
client> get name
(string) "tyler"
```

`set <key> <value>` - Sets the value of _key_. If _key_ already exists, updates its value instead.

Example:
```
client> set name tyler
(string) "OK"
client> get name
(string) "tyler"
client> set name won
(string) "OK"
client> get name
(string) "won"
```

`del <key>` - Deletes the entry for _key_.

Example:
```
client> del name
(integer) 0
client> set name tyler
(string) "OK"
client> del name
(integer) 1
```

`keys` - Gets all keys.

Example:
```
client> set firstname tyler
(string) "OK"
client> set lastname won
(string) "OK"
client> keys
(array) len=2
(string) "tyler"
(string) "won"
(array) end
```

`zadd <key> <score> <name>` - Adds a _(score, name)_ pair to the sorted set at _key_. If _key_ does not exist, a new sorted set with the specified pair is created. If a pair with _name_ already exists in the sorted set, its score is updated.

Example:
```
client> zadd myset 10 tyler
(integer) 1
client> zscore myset tyler
(double) 10.0
client> zadd myset 20 tyler
(integer) 1
client> zscore myset tyler
(double) 20.0
```

`zscore <key> <name>` - Gets the score of _name_ in the sorted set at _key_.

Example:
```
client> zadd myset 10 tyler
(integer) 1
client> zscore myset tyler
(double) 10.0
```

`zrem <key> <name>` - Removes _name_ from the sorted set at _key_.

Example:
```
client> zadd myset 10 tyler
(integer) 1
client> zrem myset tyler
(integer) 1
client> zscore myset tyler
(nil)
```

`zquery <key> <score> <name> <offset> <limit>` - Finds all pairs in the sorted set at _key_ greater than or equal to the given _(score, name)_ pair. _offset_ determines the number of pairs to exclude from the beginning of the result. _limit_ dictates the maximum number of pairs to return.

Example:
```
client> zadd myset 10 tyler
(integer) 1
client> zadd myset 20 won
(integer) 1
client> zquery myset 0 test 0 0 
(array) len=2
(double) 10.0
(string) "tyler"
(double) 20.0
(string) "won"
(array) end
client> zquery myset 0 test 1 0 
(array) len=1
(double) 20.0
(string) "won"
(array) end
client> zquery myset 0 test 0 1 
(array) len=1
(double) 10.0
(string) "tyler"
(array) end
```

`zrank <key> <name>` - Gets the rank (position in sorted order) of _name_ in the sorted set at _key_. The rank is 0-based, so the lowest pair is rank 0.

Example:
```
client> zadd myset 10 tyler
(integer) 1
client> zadd myset 20 won
(integer) 1
client> zrank myset tyler
(integer) 0
```

`expire <key> <seconds>` - Sets a timeout on _key_. After the timeout has expired, the key will be deleted. The timeout will be cleared by commands that delete or overwrite the contents of the key.

Example:
```
client> set name tyler
(string) "OK"
client> expire name 100
(integer) 1
client> ttl name
(integer) 100
client> set name won
(string) "OK"
client> ttl name
(integer) -1
```

`ttl <key>` - Gets the remaining time-to-live of _key_.

Example:
```
client> set name tyler
(string) "OK"
client> expire name 100
(integer) 1
client> ttl name
(integer) 100
```

`persist <key>` - Removes the timeout on _key_ if there is one set.

Example:
```
client> set name tyler
(string) "OK"
client> expire name 100
(integer) 1
client> persist name
(integer) 1
client> persist name
(integer) 0
```
