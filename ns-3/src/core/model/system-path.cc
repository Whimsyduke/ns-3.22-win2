/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "system-path.h"
#include "fatal-error.h"
#include "assert.h"
#include "log.h"
#include "ns3/core-config.h"
#include <cstdlib>
#include <cerrno>
#include <cstring>


///WINDOWS
//#if defined (HAVE_DIRENT_H) and defined (HAVE_SYS_TYPES_H)
///** Do we have an \c opendir function? */
//#define HAVE_OPENDIR
//#include <sys/types.h>
//#include <dirent.h>
//#endif
//#if defined (HAVE_SYS_STAT_H) and defined (HAVE_SYS_TYPES_H)
///** Do we have a \c makedir function? */
//#define HAVE_MKDIR_H
//#include <sys/types.h>
//#include <sys/stat.h>
//#endif
//发现意外的文件尾——宏定义错误。
#ifndef WIN32
#if defined (HAVE_DIRENT_H) and defined (HAVE_SYS_TYPES_H)
#define HAVE_OPENDIR
#include <sys/types.h>
#include <dirent.h>
#endif
#if defined (HAVE_SYS_STAT_H) and defined (HAVE_SYS_TYPES_H)
#define HAVE_MKDIR_H
#include <sys/types.h>
#include <sys/stat.h>
#endif
#endif
///WINDOWS

#include <sstream>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif /* __APPLE__ */

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef __linux__
#include <unistd.h>
#endif

/**
 * \def SYSTEM_PATH_SEP
 * System-specific path separator used between directory names.
 */

///WINDOWS
//#if defined (__win32__)
//#define SYSTEM_PATH_SEP "\\"
//#else
//#define SYSTEM_PATH_SEP "/"
//#endif
//避免头状态错误。参考ns-3-win2.
#if defined (WIN32)
#define SYSTEM_PATH_SEP "\\"
#include <Windows.h>
#include <WinBase.h>
#define HAVE_FIND_FIRST_FILE
#else
#if defined (__win32__)
#define SYSTEM_PATH_SEP "\\"
#else
#define SYSTEM_PATH_SEP "/"
#endif
#endif
///WINDOWS


/**
 * \file
 * \ingroup systempath
 * System-independent file and directory functions implementation.
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SystemPath");

namespace SystemPath {

/**
 * \ingroup systempath
 * \brief Get the directory path for a file.
 *
 * This is an internal function (by virtue of not being
 * declared in a \c .h file); the public API is FindSelfDirectory().
 * 
 * \param path The full path to a file.
 * \returns The full path to the containing directory.
 */
std::string Dirname (std::string path)
{
  NS_LOG_FUNCTION (path);
  std::list<std::string> elements = Split (path);
  std::list<std::string>::const_iterator last = elements.end();
  last--;
  return Join (elements.begin (), last);
}

std::string FindSelfDirectory (void)
{
  /** 
   * This function returns the path to the running $PREFIX.
   * Mac OS X: _NSGetExecutablePath() (man 3 dyld)
   * Linux: readlink /proc/self/exe
   * Solaris: getexecname()
   * FreeBSD: sysctl CTL_KERN KERN_PROC KERN_PROC_PATHNAME -1
   * BSD with procfs: readlink /proc/curproc/file
   * Windows: GetModuleFileName() with hModule = NULL
   */
  NS_LOG_FUNCTION_NOARGS ();
  std::string filename;
#if defined(__linux__)
  {
    ssize_t size = 1024;
    char *buffer = (char*)malloc (size);
    memset (buffer, 0, size);
    int status;
    while (true)
      {
        status = readlink("/proc/self/exe", buffer, size);
        if (status != 1 || (status == -1 && errno != ENAMETOOLONG))
          {
            break;
          }
        size *= 2;
        free (buffer);
        buffer = (char*)malloc (size);
	memset (buffer, 0, size);
      }
    if (status == -1)
      {
        NS_FATAL_ERROR ("Oops, could not find self directory.");
      }
    filename = buffer;
    free (buffer);
  }
#elif defined (__win32__)
  ///WINDOWS
#elif defined WIN32
  ///WINDOWS
  {
    /// \todo untested. it should work if code is compiled with
    /// LPTSTR = char *
    DWORD size = 1024;
    LPTSTR lpFilename = (LPTSTR) malloc (sizeof(TCHAR) * size);

	///WINDOWS
	//DWORD status = GetModuleFilename(0, lpFilename, size); 
// 函数名差异, 疑似bug。
#ifdef WIN32
		DWORD status = GetModuleFileName(0, lpFilename, size);
#else
		DWORD status = GetModuleFilename(0, lpFilename, size);
#endif
		///WINDOWS
    while (status == size)
      {
	size = size * 2;
	free (lpFilename);
	lpFilename = (LPTSTR)malloc(sizeof(TCHAR) * size);

	///WINDOWS
	//status = GetModuleFilename(0, lpFilename, size);
	//函数名差异,疑似bug。
#ifdef WIN32
	status = GetModuleFileName(0, lpFilename, size);
#else
	status = GetModuleFilename(0, lpFilename, size);
#endif
	///WINDOWS
      }
    NS_ASSERT (status != 0);
    filename = lpFilename;
    free (lpFilename);
  }
#elif defined (__APPLE__)
  {
    uint32_t bufsize = 1024;
    char *buffer = (char *) malloc (bufsize);
    NS_ASSERT (buffer != 0);
    int status = _NSGetExecutablePath (buffer, &bufsize);
    if (status == -1)
      {
	free (buffer);
	buffer = (char *) malloc (bufsize);
	status = _NSGetExecutablePath (buffer, &bufsize);
      }
    NS_ASSERT (status == 0);
    filename = buffer;
    free (buffer);
  } 
#elif defined (__FreeBSD__)
  {
    int     mib[4];
    size_t  bufSize = 1024;
    char   *buf = (char *) malloc(bufSize);

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PATHNAME;
    mib[3] = -1;

    sysctl(mib, 4, buf, &bufSize, NULL, 0);
    filename = buf;
  }
#endif
    return Dirname (filename);
}
  
std::string Append (std::string left, std::string right)
{
  // removing trailing separators from 'left'
  NS_LOG_FUNCTION (left << right);
  while (true)
    {
      std::string::size_type lastSep = left.rfind (SYSTEM_PATH_SEP);
      if (lastSep != left.size () - 1)
	{
	  break;
	}
      left = left.substr (0, left.size () - 1);
    } 
  std::string retval = left + SYSTEM_PATH_SEP + right;
  return retval;
}

std::list<std::string> Split (std::string path)
{
  NS_LOG_FUNCTION (path);
  std::list<std::string> retval;
  std::string::size_type current = 0, next = 0;
  next = path.find (SYSTEM_PATH_SEP, current);
  while (next != std::string::npos)
    {
      std::string item = path.substr (current, next - current);
      retval.push_back (item);
      current = next + 1;
      next = path.find (SYSTEM_PATH_SEP, current);
    }
  std::string item = path.substr (current, next - current);
  retval.push_back (item);
  return retval;
}

std::string Join (std::list<std::string>::const_iterator begin,
		  std::list<std::string>::const_iterator end)
{
  NS_LOG_FUNCTION (&begin << &end);
  std::string retval = "";
  for (std::list<std::string>::const_iterator i = begin; i != end; i++)
    {
      if (i == begin)
	{
	  retval = *i;
	}
      else
	{
	  retval = retval + SYSTEM_PATH_SEP + *i;
	}
    }
  return retval;
}
  
std::list<std::string> ReadFiles (std::string path)
{
  NS_LOG_FUNCTION (path);
  std::list<std::string> files;
#if defined HAVE_OPENDIR
  DIR *dp = opendir (path.c_str ());
  if (dp == NULL)
    {
      NS_FATAL_ERROR ("Could not open directory=" << path);
    }
  struct dirent *de = readdir (dp);
  while (de != 0)
    {
      files.push_back (de->d_name);
      de = readdir (dp);
    }
  closedir (dp);
#elif defined (HAVE_FIND_FIRST_FILE)
  /// \todo untested
  HANDLE hFind;
  WIN32_FIND_DATA fileData;

  ///WINDOWS
  //hFind = FindFirstFile (path.c_str (), &FindFileData);

  //不清楚原因，不知道FindFileData是什么,疑似bug。
#ifdef WIN32
  hFind = FindFirstFile(path.c_str(), &fileData);
#else
  hFind = FindFirstFile(path.c_str(), &FindFileData);
#endif
  ///WINDOWS
  if (hFind == INVALID_HANDLE_VALUE)
    {
      NS_FATAL_ERROR ("Could not open directory=" << path);
    }
  do
    {
      files.push_back (fileData.cFileName);
    } while (FindNextFile (hFind, &fileData));
  FindClose(hFind);
#else
#error "No support for reading a directory on this platform"
#endif
  return files;
}

std::string 
MakeTemporaryDirectoryName (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  char *path = NULL;

  path = getenv ("TMP");
  if (path == NULL)
    {
      path = getenv ("TEMP");
      if (path == NULL)
	{
	  path = const_cast<char *> ("/tmp");
	}
    }

  //
  // Just in case the user wants to go back and find the output, we give
  // a hint as to which dir we created by including a time hint.
  //

  ///WINDOWS
  //time_t now = time (NULL);
  //struct tm *tm_now = localtime(&now);
  //参考ns-3-win2
#ifndef WIN32
  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
#endif
  ///WINDOWS

  //
  // But we also randomize the name in case there are multiple users doing
  // this at the same time
  //

  ///WINDOWS
  //srand (time (0));
  //long int n = rand();
  //参考ns-3-win2
#ifndef WIN32
  srand(time(0));
  long int n = rand();
#endif
  ///WINDOWS

  //
  // The final path to the directory is going to look something like
  // 
  //   /tmp/ns3-14.30.29.32767
  //
  // The first segment comes from one of the temporary directory env 
  // variables or /tmp if not found.  The directory name starts with an
  // identifier telling folks who is making all of the temp directories
  // and then the local time (in this case 14.30.29 -- which is 2:30 and
  // 29 seconds PM).
  //
  std::ostringstream oss;

  ///WINDOWS
  //oss << path << SYSTEM_PATH_SEP << "ns-3." << tm_now->tm_hour << "."
	 // << tm_now->tm_min << "." << tm_now->tm_sec << "." << n;
  //参考ns-3-win2.
#ifndef WIN32
  oss << path << SYSTEM_PATH_SEP << "ns-3." << tm_now->tm_hour << "."
	  << tm_now->tm_min << "." << tm_now->tm_sec << "." << n;
#else
  //This may need to be fixed.  Used?
  oss << path << SYSTEM_PATH_SEP << "ns-3";
#endif
  ///WINDOWS

  return oss.str ();
}

void 
MakeDirectories (std::string path)
{
  NS_LOG_FUNCTION (path);

  // Make sure all directories on the path exist
  std::list<std::string> elements = Split (path);
  for (std::list<std::string>::const_iterator i = elements.begin (); i != elements.end (); ++i)
    {
      std::string tmp = Join (elements.begin (), i);

	  ///WINDOWS
#ifdef WIN32
	  if(!CreateDirectory(tmp.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS && tmp.size() != 0)
	  {
		  NS_LOG_ERROR("failed creating directory " << tmp);
}
#endif
	  ///WINDOWS

#if defined(HAVE_MKDIR_H)
      if (mkdir (tmp.c_str (), S_IRWXU))
        {
          NS_LOG_ERROR ("failed creating directory " << tmp);
        }
#endif
  }

  ///WINDOWS
#ifdef WIN32
  if(!CreateDirectory(path.c_str (), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
  {
	  NS_LOG_ERROR("failed creating directory " << path);
	  }
#endif
  ///WINDOWS

  // Make the final directory.  Is this redundant with last iteration above?
#if defined(HAVE_MKDIR_H)
  if (mkdir (path.c_str (), S_IRWXU))
    {
      NS_LOG_ERROR ("failed creating directory " << path);
    }
#endif

}

} // namespace SystemPath

} // namespace ns3
