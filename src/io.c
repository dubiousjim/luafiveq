/*
 * io.c: 5.2-like Enhancements to os and io libraries
 *
 * 1. os.execute() ~~> bool: is a shell available?
 * 2. os.execute(...) and io.popen(...):close() now return: (true or nil, "exit" or "signal", exit code or signal)
 
 * 3. io.open sanitizes mode string
 * 4. io.lines, file:lines passes arguments through to read, default to "*l"
 * 5. io.read, file:read now accept "*L" argument
 * 6. file:write now returns file
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "fiveq.h"


/* --- adapted from loslib.c and lauxlib.c --- */

static int pushresult (lua_State *L, int stat, const char *filename) {
  int en = errno;  /* calls to Lua API may change this value */
  if (stat) {
    lua_pushboolean(L, 1);
    return 1;
  }
  else {
    lua_pushnil(L);
    if (filename)
      lua_pushfstring(L, "%s: %s", filename, strerror(en));
    else
      lua_pushfstring(L, "%s", strerror(en));
    lua_pushinteger(L, en);
    return 3;
  }
}

#include <sys/wait.h>

/*
** use appropriate macros to interpret 'pclose' return status
*/
#define inspectstat(stat,what)  \
   if (WIFEXITED(stat)) { stat = WEXITSTATUS(stat); } \
   else if (WIFSIGNALED(stat)) { stat = WTERMSIG(stat); what = "signal"; }


static int luaL_execresult (lua_State *L, int stat) {
  const char *what = "exit";  /* type of termination */
  if (stat == -1)  /* error? */
    return pushresult(L, 0, NULL);
  else {
    inspectstat(stat, what);  /* interpret result */
    if (*what == 'e' && stat == 0)  /* successful termination? */
      lua_pushboolean(L, 1);
    else
      lua_pushnil(L);
    lua_pushstring(L, what);
    lua_pushinteger(L, stat);
    return 3;  /* return true/nil,what,code */
  }
}


/* --- adapted from loslib.h --- */

static int os_execute (lua_State *L) {
  const char *cmd = luaL_optstring(L, 1, NULL);
  int stat = system(cmd);
  if (cmd != NULL)
    return luaL_execresult(L, stat);
  else {
    lua_pushboolean(L, stat);  /* true if there is a shell */
    return 1;
  }
}

/*
static int os_exit (lua_State *L) {
  int status;
  if (lua_isboolean(L, 1))
    status = (lua_toboolean(L, 1) ? EXIT_SUCCESS : EXIT_FAILURE);
  else
    status = luaL_optint(L, 1, EXIT_SUCCESS);
  if (lua_toboolean(L, 2))
    lua_close(L);
  exit(status);
}
*/


/* --- adapted from liolib.h and luaconf.h --- */


#define IO_INPUT	1
#define IO_OUTPUT	2

#define tofilep(L)	((FILE **)luaL_checkudata(L, 1, LUA_FILEHANDLE))


static FILE *tofile (lua_State *L) {
  FILE **f = tofilep(L);
  if (*f == NULL)
    luaL_error(L, "attempt to use a closed file");
  return *f;
}


/*
** When creating file handles, always creates a `closed' file handle
** before opening the actual file; so, if there is a memory error, the
** file is not left opened.
*/
static FILE **newfile (lua_State *L) {
  FILE **pf = (FILE **)lua_newuserdata(L, sizeof(FILE *));
  *pf = NULL;  /* file handle is currently `closed' */
  luaL_getmetatable(L, LUA_FILEHANDLE);
  lua_setmetatable(L, -2);
  return pf;
}


/*
** function to close 'popen' files
*/
static int io_pclose (lua_State *L) {
  FILE **p = tofilep(L);
  int stat = ((void)L, (pclose(*p)));
  *p = NULL;
  return luaL_execresult(L, stat);
}



static int io_open (lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  const char *mode = luaL_optstring(L, 2, "r");
  FILE **pf = newfile(L);
  int i = 0;
  /* check whether 'mode' matches '[rwa]%+?b?' */
  if (!(mode[i] != '\0' && strchr("rwa", mode[i++]) != NULL &&
       (mode[i] != '+' || ++i) &&  /* skip if char is '+' */
       (mode[i] != 'b' || ++i) &&  /* skip if char is 'b' */
       (mode[i] == '\0')))
    return luaL_error(L, "invalid mode " LUA_QL("%s")
                         " (should match " LUA_QL("[rwa]%%+?b?") ")", mode);
  *pf = fopen(filename, mode);
  return (*pf == NULL) ? pushresult(L, 0, filename) : 1;
}


static int io_readline (lua_State *L);


static void aux_lines (lua_State *L, int toclose) {
  int i;
  int n = lua_gettop(L) - 1;  /* number of arguments to read */
  /* ensure that arguments will fit here and into 'io_readline' stack */
  luaL_argcheck(L, n <= LUA_MINSTACK - 3, LUA_MINSTACK - 3, "too many options");
  lua_pushvalue(L, 1);  /* file handle */
  lua_pushboolean(L, toclose);  /* close/not close file when finished */
  lua_pushinteger(L, n);  /* number of arguments to read */
  for (i = 1; i <= n; i++) lua_pushvalue(L, i + 1);  /* copy arguments */
  lua_pushcclosure(L, io_readline, 3 + n);
}


static int f_lines (lua_State *L) {
  tofile(L);  /* check that it's a valid file handle */
  aux_lines(L, 0); /* do not close it after iteration */
  return 1;
}


static int io_lines (lua_State *L) {
   int toclose;
   if (lua_isnone(L, 1))
     lua_pushnil(L);  /* at least one argument */
   if (lua_isnil(L, 1)) {  /* no file name? */
     /* will iterate over default input */
     lua_rawgeti(L, LUA_ENVIRONINDEX, IO_INPUT);
     lua_replace(L, 1);  /* put it at index 1 */
     tofile(L);  /* check that it's a valid file handle */
     toclose = 0;  /* do not close it after iteration */
  }
   else {  /* open a new file */
    const char *filename = luaL_checkstring(L, 1);
    FILE **pf = newfile(L);
    *pf = fopen(filename, "r");
    if (*pf == NULL) {
      lua_pushfstring(L, "%s: %s", filename, strerror(errno));
      luaL_argerror(L, 1, lua_tostring(L, -1));
    }
    lua_replace(L, 1);  /* put file at index 1 */
    toclose = 1;  /* close it after iteration */
  }
   aux_lines(L, toclose);
   return 1;
}



static int read_number (lua_State *L, FILE *f) {
  lua_Number d;
  if (fscanf(f, LUA_NUMBER_SCAN, &d) == 1) {
    lua_pushnumber(L, d);
    return 1;
  }
  else return 0;  /* read fails */
}


static int test_eof (lua_State *L, FILE *f) {
  int c = getc(f);
  ungetc(c, f);
  lua_pushlstring(L, NULL, 0);
  return (c != EOF);
}


static int read_line (lua_State *L, FILE *f, int chop) {
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  for (;;) {
    size_t l;
    char *p = luaL_prepbuffer(&b);
    if (fgets(p, LUAL_BUFFERSIZE, f) == NULL) {  /* eof? */
      luaL_pushresult(&b);  /* close buffer */
      return (lua_objlen(L, -1) > 0);  /* check whether read something */
    }
    l = strlen(p);
    if (l == 0 || p[l-1] != '\n')
      luaL_addsize(&b, l);
    else {
      luaL_addsize(&b, l - chop);  /* chop 'eol' if needed */
      luaL_pushresult(&b);  /* close buffer */
      return 1;  /* read at least an `eol' */
    }
  }
}


static int read_chars (lua_State *L, FILE *f, size_t n) {
  size_t rlen;  /* how much to read */
  size_t nr;  /* number of chars actually read */
  char *p;
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  rlen = LUAL_BUFFERSIZE;  /* try to read that much each time */
  do {
    p = luaL_prepbuffer(&b);
    if (rlen > n) rlen = n;  /* cannot read more than asked */
    nr = fread(p, sizeof(char), rlen, f);
    luaL_addsize(&b, nr);
    n -= nr;  /* still have to read `n' chars */
  } while (n > 0 && nr == rlen);  /* until end of count or eof */
  luaL_pushresult(&b);  /* close buffer */
  return (n == 0 || lua_objlen(L, -1) > 0);
}


static int g_read (lua_State *L, FILE *f, int first) {
  int nargs = lua_gettop(L) - 1;
  int success;
  int n;
  clearerr(f);
  if (nargs == 0 || lua_isnil(L, first)) {  /* no arguments? */
    success = read_line(L, f, 1);
    n = first+1;  /* to return 1 result */
  }
  else {  /* ensure stack space for all results and for auxlib's buffer */
    luaL_checkstack(L, nargs+LUA_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (lua_type(L, n) == LUA_TNUMBER) {
        size_t l = (size_t)lua_tointeger(L, n);
        success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
      }
      else {
        const char *p = lua_tostring(L, n);
        luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
        switch (p[1]) {
          case 'n':  /* number */
            success = read_number(L, f);
            break;
          case 'l':  /* line */
            success = read_line(L, f, 1);
            break;
          case 'L':  /* line with end-of-line */
            success = read_line(L, f, 0);
            break;
          case 'a':  /* file */
            read_chars(L, f, ~((size_t)0));  /* read MAX_SIZE_T chars */
            success = 1; /* always success */
            break;
          default:
            return luaL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (ferror(f))
    return pushresult(L, 0, NULL);
  if (!success) {
    lua_pop(L, 1);  /* remove last result */
    lua_pushnil(L);  /* push nil instead */
  }
  return n - first;
}


static int io_read (lua_State *L) {
  FILE *f;
  lua_rawgeti(L, LUA_ENVIRONINDEX, IO_INPUT);
  f = *(FILE **)lua_touserdata(L, -1);
  if (f == NULL)
    return luaL_error(L, "standard input file is closed");
  return g_read(L, f, 1);
}


static int f_read (lua_State *L) {
  return g_read(L, tofile(L), 2);
}



/* upvalue[1]=file; upvalue[2]=created file?; upvalue[3]=number of saved args; upvalues[4+]=args... */
static int io_readline (lua_State *L) {
   FILE *f = *(FILE **)lua_touserdata(L, lua_upvalueindex(1));
   if (f == NULL)  /* file is already closed? */
     luaL_error(L, "file is already closed");
   lua_settop(L, 1);
   int n = (int)lua_tointeger(L, lua_upvalueindex(3));
   int i;
   for (i = 1; i <= n; i++)  /* push arguments to 'g_read' */
     lua_pushvalue(L, lua_upvalueindex(3 + i));
   n = g_read(L, f, 2);  /* 'n' is number of results */
   lua_assert(n > 0);  /* should return at least a nil */
   if (!lua_isnil(L, -n))  /* read at least one value? */
     return n;  /* return them */
   else {  /* first result is nil: EOF or error */
     if (!lua_isnil(L, -1))  /* is there error information? */
       return luaL_error(L, "%s", lua_tostring(L, -1));  /* error */
     /* else EOF */
     if (lua_toboolean(L, lua_upvalueindex(2))) {  /* generator created file? */
      lua_settop(L, 0);
      lua_pushvalue(L, lua_upvalueindex(1));
      /* close it */
      lua_getfenv(L, 1);
      lua_getfield(L, -1, "__close");
      return (lua_tocfunction(L, -1))(L);
    }
    return 0;
  }
}


static int f_write (lua_State *L) {
  FILE *f = tofile(L);
  int arg = 2;
  int nargs = lua_gettop(L) - 1;
  int status = 1;
  for (; nargs--; arg++) {
    if (lua_type(L, arg) == LUA_TNUMBER) {
      /* optimization: could be done exactly as for strings */
      status = status &&
          fprintf(f, LUA_NUMBER_FMT, lua_tonumber(L, arg)) > 0;
    }
    else {
      size_t l;
      const char *s = luaL_checklstring(L, arg, &l);
      status = status && (fwrite(s, sizeof(char), l, f) == l);
    }
  }
  if (status) {
    lua_pushvalue(L, 1);
    return 1;
  }
  else return pushresult(L, status, NULL);
}



/*
** functions for 'io' library
*/
static const luaL_Reg iolib[] = {
  {"open", io_open},
  {"lines", io_lines},
  {"read", io_read},
  {NULL, NULL}
};


/*
** methods for file handles
*/
static const luaL_Reg flib[] = {
  {"lines", f_lines},
  {"read", f_read},
  {"write", f_write},
  {NULL, NULL}
};

#ifndef LUA_FIVEQ_PLUS
extern void luaQ_checklib (lua_State *L, const char *libname);

#define luaQ_register(L,idx,name,f)  do { lua_pushcfunction(L, (f)); \
    if ((idx) < 0) lua_setfield(L, (idx)-1, (name)); \
    else if ((idx) > 0) lua_setfield(L, (idx), (name)); \
    else lua_setglobal(L, (name)); } while (0)
#endif

extern int luaopen_fiveq_io (lua_State *L) {
  luaQ_checklib(L, LUA_IOLIBNAME);
  luaL_getmetatable(L, LUA_FILEHANDLE);
  if (lua_isnil(L, -1))
    return luaL_error(L, "can't open %s metatable", LUA_FILEHANDLE);
  luaL_setfuncs(L, flib, 0);  /* replacement file methods */
  lua_getfield(L, -2, "close");
  lua_getfenv(L, -1);  /* this library will only be built under 5.1 */ 
  lua_replace(L, LUA_ENVIRONINDEX); /* we also use the io lib's fenv */
  lua_pop(L, 2);
  luaL_setfuncs(L, iolib, 0);  /* replacement library methods */
  lua_getfield(L, -1, "popen");
  lua_getfenv(L, -1);
  luaQ_register(L, -1, "__close", io_pclose); /* replace the popen().__close method */
  lua_pop(L, 3);
  luaQ_checklib(L, LUA_OSLIBNAME);
  luaQ_register(L, -1, "execute", os_execute); /* replace os.execute method */
  return 0;
}
