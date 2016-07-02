# webserver
A simple HTTP web server with an event-driven architecture and a fixed-size thread pool.

Supports both HTTP/1.0 and HTTP/1.1.
As the focus of this project was server performance rather than usability, however, the server only 
  supports enough of each protocol to serve [this static webpage](http://sysnet.cs.williams.edu/).
The contents of this webpage in the subdirectory `resources`.

`ab` load tests distributed over 20 Amazon EC2 instances indicate comparable performance to an Apache server on up to 1200 concurrent connections.  A different architecture is necessary, however, for successful handling of additional connections.  See the subdirectory `writeup` for further details about implementation and performance.

## To run
Run `make` in the project root directory, then run `server -port <portno>` with the port number of your choice.
