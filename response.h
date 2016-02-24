#ifndef __RESPONSE_H__
#define __RESPONSE_H__

extern void send_response(int clientfd, int status);

extern void send_initial(int clientfd, int status);
extern char *status_to_phrase(int status);

extern void send_date_header(int clientfd);
extern void send_contenttype_header(int clientfd);
extern void send_contentlen_header(int clientfd);

#endif  // __RESPONSE_H__
