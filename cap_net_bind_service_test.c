#include <sys/capability.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>
 
int main() {
  char str[100];
  int listen_fd, comm_fd;
  struct sockaddr_in servaddr;
  struct passwd *pw = NULL;
  cap_value_t cap_values[] = {CAP_NET_BIND_SERVICE};
  cap_t caps;

  if(getgid() && getuid()) {
    printf("Privileges are not dropped as we're not superuser.\n");
    exit(EXIT_FAILURE);
  }

  /* Add the capability of interest to the perimitted capabilities  */
  caps = cap_get_proc();
  cap_set_flag(caps, CAP_PERMITTED, 1, cap_values, CAP_SET);
  cap_set_proc(caps);

  /* Tell the kernel to retain permitted capabilities */
  if(prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) != 0) {
    printf("Unable to retain permitted capabilities [%s]\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* Cleanup */
  cap_free(caps);

  /* Drop privileges to user nobody */
  pw = getpwnam("nobody");
  if(pw == NULL) {
    printf("Unable to get information for user nobody");
    exit(EXIT_FAILURE);
  }

  /* Drop privileges */
  if((setgid(pw->pw_gid) != 0) || (setuid(pw->pw_uid) != 0)) {
    printf("Unable to drop privileges [%s]\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* Double check that privileges have been dropped */
  if (setuid(0) != -1) {
    printf("Managed to regain root privileges?\n");
    exit(EXIT_FAILURE);
  }

  /* Add the new capabilities to the effective capabilities
     Note that PR_SET_KEEPCAPS only preserves permitted capabilities
     so it is necessary to do this step
  */
  caps = cap_get_proc();
  cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_values, CAP_SET);
  cap_set_proc(caps);
  /* cap_to_text shoud output, among other text, the string
     cap_net_bind_service+ep to confirm the capability
     is effective (e)  and permitted (p)
     printf("caps_to_text() returned \"%s\"\n", cap_to_text(caps, NULL));
  */
  cap_free(caps);

  /* The following is just an echo server
     to test that nobody can actually bind and
     listen to sockets on ports < 1024
  */

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  bzero( &servaddr, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(81); /* Use a port < 1024 to test the effectiveness of this script */

  if(bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1
     || listen(listen_fd, 10) == -1){
    printf("Something went wrong with bind() or listen() [%s]\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
 
  comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL); 
  while(1) {
    bzero( str, 100);
    read(comm_fd,str,100);
    printf("Echoing back - %s",str);
    write(comm_fd, str, strlen(str)+1);
  }
}
