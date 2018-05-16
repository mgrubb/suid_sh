#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>

  
int main(int argc, char **argv, char **envp) {
  if (setgid(getegid())) perror("setgid");
  if (setuid(geteuid())) perror("setuid");
  execve("/usr/local/bin/SOMESCRIPTHERE", argv, envp);
  perror(argv[0]);
  return errno;
}
