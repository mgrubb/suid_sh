#include <config.h>

#define EUSER_SIZE 256
#define EGROUP_SIZE 256
#define SETUID 1
#define SETGID 2

struct config_entry {
  char wrapper[MAXPATHLEN];
  char path[MAXPATHLEN];
  int linenum;
  char euser[EUSER_SIZE];
  char egroup[EGROUP_SIZE];
  uid_t euid;
  gid_t egid;
  int setugid;
};

#ifdef DEBUG
#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug(...) ((void)0)
#endif /* defined DEBUG */

#ifdef DEBUG
#define BEGIN_DEBUG
#define END_DEBUG
#else
#define BEGIN_DEBUG while(0) {
#define END_DEBUG }
#endif /* defined DEBUG */


const char CFSEP[] = " \t";

void print_version_info(void) {
  char *envopt;
  envopt = getenv("SUIDSH_VERSION");
  if (envopt == NULL || envopt[0] == '\0')
    return;
  
  fprintf(stderr, "SUIDSH Version %d.%d.%d\n", SUIDSH_VERSION_MAJOR, SUIDSH_VERSION_MINOR, SUIDSH_VERSION_PATCH);
  fprintf(stderr, "Configuration File %s\n", CONFIG_LOCATION);
  exit(0);
}

void fatal(char *format, ...) {
  char *nf;
  va_list argList;
  va_start(argList, format);
  if ((nf = malloc(strlen(format) + 9)) == NULL) {
    fprintf(stderr, "Memory allocation error in fatal: %s\n", format);
    exit(1);
  }
  strcat(nf, "suidsh: ");
  strcat(nf, format);
  vfprintf(stderr, nf, argList);
  perror("suidsh");
  free(nf);
  exit(1);
}

void append_path(char *p, char *p1, int len) {
  int plen = strlen(p);
  char *e = p + plen;
  *(e++) = '/';
  strncat(p, p1, len - plen - 1);
}

int file_exists(char *path) {
  struct stat st;
  return !stat(path, &st);
}

#ifdef DEBUG
#define debug_token(ty,t,r) fprintf(stderr, "Type: (%s), Token: (%s) Rest: (%s)\n", ty, t, r)
#else
#define debug_token(ty,t,r) ((void)0)
#endif

int parse_config_line(char *line, struct config_entry *ce) {
  char *token, *tmp;
  token = tmp = strdup(line);
  if (tmp == NULL)
    fatal("Memory allocation error when parsing configuration line: %d:%s\n", ce->linenum, line);

  /* get wrapper path */
  token = strsep(&tmp, CFSEP);
  debug_token("WRAPPER", token, tmp);
  if (token == NULL)
    return 1;
  else
    strncpy(ce->wrapper, token, MAXPATHLEN);
  
  /* get script path */
  token = strsep(&tmp, CFSEP); 
  debug_token("PATH", token, tmp);
  if (token == NULL)
    return 1;
  else
    strncpy(ce->path, token, MAXPATHLEN);

  /* get script user */
  token = strsep(&tmp, " \t");
  debug_token("USER", token, tmp);
  if (token == NULL)
    return 1;
  else
    strncpy(ce->euser, token, EUSER_SIZE);

  /* get script group */
  token = strsep(&tmp, " \t");
  debug_token("GROUP", token, tmp);
  if (token == NULL) {
  strncpy(ce->egroup, "-", EGROUP_SIZE);
}
  else {
    strncpy(ce->egroup, token, EGROUP_SIZE);
  }

  return 0;
}

char *search_path(char *path) {
  char fqp[MAXPATHLEN];
  int len;
  char *envpath = getenv("PATH");
  char *token, *pathcpy, *m;


  if (envpath == NULL) {
    fprintf(stderr, "suidsh: Could not get PATH environment variable\n");
    perror("suidsh");
    exit(1);
  }

  m = pathcpy = token = strdup(envpath);
  if (pathcpy == NULL) {
    fprintf(stderr, "suidsh: Memory allocation error when copying PATH variable\n");
    perror("suidsh");
    exit(1);
  }

  while ((token = strsep(&pathcpy, ":")) != NULL) {
    memset(fqp, 0, sizeof(fqp));
    debug("Searching PATH: (%s) %s\n", pathcpy, token);
    len = MAXPATHLEN;
    strncpy(fqp, token, len);
    len -= strlen(token);
    append_path(fqp, path, len);
    if (file_exists(fqp)) {
      debug("Found path(%s) at %s\n", path, fqp);
      free(m);
      return strdup(fqp);
    }
  }
  free(m);
  return NULL;
}

char *fqpn(char *path) {
  char fqp[MAXPATHLEN];
  int fqplen = 0;
  int cwd = -1;
  char *p, *p1, *tmp;
  memset(fqp, 0, sizeof(fqp));
  
  tmp = strdup(path);
  if (tmp == NULL) {
    fprintf(stderr, "suidsh: Memory allocation error in string duplication(fqpn)\n");
    perror("suidsh");
    exit(1);
  }

  p = strrchr(tmp, '/');
  debug("FQPN(): Search for /: (%s) (%s)\n", p, tmp);
  if (p == NULL)
    return search_path(path);

  *p = '\0';

  cwd = open(".", O_RDONLY);
  if (cwd < 0) {
    fprintf(stderr, "suidsh: Could not open current directory\n");
    perror("suidsh");
    goto error;
  }

  if (chdir(tmp)) {
    fprintf(stderr, "suidsh: Could not find directory %s\n", tmp);
    perror("suidsh");
    goto error;
  }

  if (getcwd(fqp, sizeof(fqp)) == NULL) {
    fprintf(stderr, "suidsh: Could not get working directory name\n");
    perror("suidsh");
    goto error;
  }
  fchdir(cwd);
  close(cwd);

  append_path(fqp, (p + 1), sizeof(fqp));

  debug("suidsh: FQPN: %s\n", fqp);

  return strdup(fqp); 

error:
  if (tmp != NULL)
    free(tmp);

  if (cwd >= 0) {
    fchdir(cwd);
    close(cwd);
  }

  exit(1);
}

char *ltrim(char *s) {
  while (*s == ' ' || *s == '\t')
    ++s;
  return s;
}

int check_uid(struct config_entry *ce) {
  struct stat st;
  uid_t cfuid;
  gid_t cfgid;
  struct passwd *uinfo;
  struct group *ginfo;
  if (stat(ce->path, &st)) {
    fprintf(stderr, "SETUID Script file %s not found\n", ce->path);
    return 0;
  }

  if (!(st.st_mode & S_ISUID) && !(st.st_mode & S_ISGID)) {
    fprintf(stderr, "Permission denied. Script %s is neither setuid nor setgid\n", ce->path);
    return 0;
  }
  
  if (st.st_mode & S_ISUID) {
    if (strcmp(ce->euser, "-") == 0) {
      fprintf(stderr, "Permission denied. File is SETUID, but no user given in configuration file\n");
      return 0;
    }
    if ((uinfo = getpwnam(ce->euser)) == NULL)
      fatal("Could not get UID of user %s\n", ce->euser);

    ce->euid = uinfo->pw_uid;
    ce->setugid |= SETUID;
    if (ce->euid != st.st_uid) {
      fprintf(stderr, "Permission denied. Allowed user (%d) does not match actual user (%d) of %s\n", ce->euid, st.st_uid, ce->path);
      return 0;

    }
  }

  if (st.st_mode & S_ISGID) {
    if (strcmp(ce->egroup, "-") == 0) {
      fprintf(stderr, "Permission denied. File is SETGID, but no group given in configuration file\n");
      return 0;
    }
    if ((ginfo = getgrnam(ce->egroup)) == NULL)
      fatal("Could not get GID of group %s\n", ce->egroup);

    ce->egid = ginfo->gr_gid;
    ce->setugid |= SETGID;
    if (ce->egid != st.st_gid) {
      fprintf(stderr, "Permission denied. Allowed group (%d) does not match actual group (%d) of %s\n", ce->egid, st.st_gid, ce->path);
      return 0;
    }
  }


  return 1;
}

struct config_entry *check_allowed(char *prog) {
  FILE *cfh;
  int found = 0, count = 0;
  char cfline[MAXPATHLEN + EUSER_SIZE + EGROUP_SIZE + 50];
  struct config_entry *ce;
  struct stat st;
  char *fqp, *p;

  if ((ce = malloc(sizeof(struct config_entry))) == NULL)
    fatal("Memory allocation error while checking permissions\n");

  memset(cfline, 0, sizeof(cfline));

  fqp = fqpn(prog);
  if (fqp == NULL) {
    fprintf(stderr, "suidsh: Could not resolve program path: %s\n", prog);
    exit(1);
  }

  if (stat(CONFIG_LOCATION, &st)) {
    free(ce);
    free(fqp);
    fatal("Configuration file could not be found\n");
  }
  else if (st.st_uid != 0) {
    free(ce);
    free(fqp);
    fatal("Configuration file not owned by root\n");
  }
  else if ((st.st_mode & S_IWGRP) || (st.st_mode & S_IWOTH)) {
    free(ce);
    free(fqp);
    fatal("Configuration file is writable by group or other\n");
  }

  cfh = fopen(CONFIG_LOCATION, "r");
  if (cfh == NULL) {
    fprintf(stderr, "suidsh: Could not open configuration file %s\n", CONFIG_LOCATION);
    perror("suidsh");
    free(fqp);
    exit(1);
  }

  while (fgets(cfline, sizeof(cfline), cfh) != NULL) {
    memset(ce, 0, sizeof(struct config_entry));
    ce->linenum = ++count;
    p = ltrim(cfline);
    if (p[0] == '#' || p[0] == '\r' || p[0] == '\n')
      continue;
    
    p[strlen(p)-1] = '\0';
    if (parse_config_line(cfline, ce)) {
      fprintf(stderr, "Ignoring Malformed configuration entry %s:%d:%s\n", CONFIG_LOCATION, count, cfline);
      continue;
    }

    debug("Conf Line: %s\n", p);
    debug("strcmp(\"%s\", \"%s\"); == %d\n", ce->wrapper, fqp, strcmp(ce->wrapper, fqp));

    if (strcmp(ce->wrapper, fqp) == 0) {
      if (!check_uid(ce))
        found = 0;
      else
        found = 1;

      debug("Found: %d, ce->wrapper: %s, ce->path: %s, ce->euser: %s, ce->egroup: %s, ce->setugid: %d, ce->euid: %d, ce->egid: %d\n",
            found, ce->wrapper, ce->path, ce->euser, ce->egroup, ce->setugid, ce->euid, ce->egid);
      goto done;
    }
  }

done:
  fclose(cfh);
  if (found) {
    return ce;
  }
  else {
    free(ce);
    return NULL;
  }
}
  
int main(int argc, char **argv, char **envp) {
  char script[MAXPATHLEN], *p;
  char *envopt;
  struct config_entry *ce;
  int len, s;
  int i;

  print_version_info();

  BEGIN_DEBUG
    debug("Real UID: %d\nReal GID: %d\nEffective UID: %d\nEffective GID: %d\n", getuid(), getgid(), geteuid(), getegid());
    debug("Argv:");
    for (i = 0; i < argc; ++i) {
      debug(" %d=%s", i, argv[i]);
    }
    debug("\n");
  END_DEBUG

  if ((ce = check_allowed(argv[0])) == NULL) {
    fprintf(stderr, "suidsh: Program '%s' not allowed to run suid. Check path in configuration %s\n", argv[0], CONFIG_LOCATION);
    exit(1);
  }

  if (ce->setugid & SETUID) {
    debug("ce->setugid has SETUID bit set\n");
    if (setuid(ce->euid)) fatal("Could not SETUID\n");
  }
  else {
    if (seteuid(getuid())) fatal("Could not revert to effective user id\n");
  }

  if (ce->setugid & SETGID) {
    debug("ce->setugid has SETGID bit set\n");
    if (setgid(ce->egid)) fatal("Could not SETGID\n");
  }
  else {
    if (setegid(getgid())) fatal("Could not revert to effective group id\n");
  }

  debug("After SET{U,G}ID Calls:\n");
  debug("Real UID: %d\nReal GID: %d\nEffective UID: %d\nEffective GID: %d\n", getuid(), getgid(), geteuid(), getegid());

  argv[0] = ce->path;

  BEGIN_DEBUG
    debug("New Argv:");
  for (i = 0; i < argc; ++i) {
    debug(" %d=%s", i, argv[i]);
  }
  debug("\n");
  END_DEBUG

  execve(argv[0], argv, envp);
  perror(argv[0]);
  return errno;
}
