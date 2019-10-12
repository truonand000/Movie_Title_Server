## Running QueryClient

```
./queryclient localhost 1500
```

where **localhost** can be replaced with another IP address,
and **1500** can be replaced with another port number.

The port number must be the port that the server is listening on. 

## Running QueryServer

```
./queryserver ../data/ 1500
```

where **../data/** can be replaced with any path to a data directory,
and **1500** can be replaced with any port you want the server to listen on.

**NOTE:** The server starts listening on the specified port, and the
client must connect to that port.

## Running MultiServer

```
./multiserver ../data/ 1500
```

This is run just the same as queryserver is run;

**../data/** can be replaced with any data directory.

**1500** can be replaced with any port you want the server to listen on.

