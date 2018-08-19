# Simple Server
I wanted to make a simple socket listener that would accept incoming connections,
say "Hello World", and hangup. No frills, no bells, no whistles.

The initial pass at this follows Beej's simple server example from chpter 6
pretty closely, except that it's an even more dumbed down example. The code is
in `server.cpp` and is pretty straightforward to interact with.

Build and run:
```
g++ -o server server.cpp
./servre 8080
```

Test it out by connecting with `netcat`:
```
netcat localhost 8080
```

## IPv6 and Dual-Stack'ing 
When I initially wrote this code, I figured that I would bind to both `0.0.0.0`
and `::`, so that I would be listening on both IPv4 and IPv6. To do this I
wrote the create, bind, and listen loop like this:
```
memset(&hints, 0, sizeof hints); // Zero out our struct
hints.ai_family   = AF_UNSPEC;   // Both IPv4 and IPv6
hints.ai_socktype = SOCK_STREAM; // TCP
hints.ai_flags    = AI_PASSIVE;  // Serves on 0.0.0.0 and ::

...

for ( p = res; p != NULL; p = p->ai_next ) {
    inet_ntop(p->ai_family, get_addr_ptr(p->ai_addr), ipstr, sizeof ipstr);
    printf("Found IP: %s\n",ipstr);
    
    printf("\tGetting socket...\t");
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if ( sockfd == -1 ) {
        printf("FAILED!\n");
        perror("\t\tSocket");
        continue;
    }
    printf("OK\n");

    printf("\tBind Socket to Port...\t");
    if ( bind(sockfd, p->ai_addr, p->ai_addrlen) == -1 ) {
        close(sockfd);
        printf("FAILED!\n");
        perror("\t\tBind");
        continue;
    }
    printf("OK\n");

    printf("\tListen on socket...\t");
    if ( listen(sockfd, BACKLOG) == -1 ){
        printf("FAILED!\n");
        perror("\t\tError");
        continue;
    }
    printf("OK\n");
    break;
}
```

When I ran the above code, I would get this output:
```
$ ./server 8080
Found IP: 0.0.0.0
    Getting socket...       OK
    Bind Socket to Port...  OK
    Listen on socket...     OK
Found IP: ::
    Getting socket...       OK
    Bind Socket to Port...  FAILED
        Error: Address already in use
```
I then looked at `netstat`, and didn't see any conflicts?

I then wrote the loop to skip the `0.0.0.0` connection, and found that I could
indeed bind to `::` on port `8080`...

So... why? I am creating two sockets, with different addresses. Why would
`0.0.0.0:8080` and `:::80` conflict?

Thanks to the kindness of strangers, I was able to get [an answer on StackOverflow](https://stackoverflow.com/a/51913093/2566561).
It turns out that, at least on most Linux systems, sockets have the ability to
run dual-stack'ed, meaning that a single socket can handle both IP protocols.
In many Linux distros, this ability is on by default, and using `::` is actually
considered to mean any address. In this configuration, the socket will handle
IPv6 like you would expect, but will also accept IPv4 connections by mapping
them to `::ffff:<ipv4_addr>`.

So lets test this out! Take a look at `dualStackServer.cpp`. We can set the
address family to `AI_INET6`, and explicitly state that we want the socket to
NOT be IPv6 only via the `IPV6_V6ONLY` option. Like I previously said, this seems
to be the default on most Linux systems, but the explicit route is almost ALWAYS
better. When we run the `dualStackServer` code, we see that we have created a
socket on `::`, and are listening on it. If we try to connect to the code via
`netcat` and IPv4, e.g. `netcat 192.168.1.149 8080`, we can see that the 
connection gets mapped through!
```
$ ./dualStackServer 8080
Found IP: ::
    Getting socket...   OK
    Bind Socket to Port...  OK
    Listen on socket... OK
Connection from: ::ffff:192.168.1.149
``` 




