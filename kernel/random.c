#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define INIT_SEED 0x2A


struct {
  struct spinlock lock;
  uint8 lfsr;

  #define RANDOM_BUF_SIZE 128
  char buf[RANDOM_BUF_SIZE];

} rand;

// Linear feedback shift register
// Returns the next pseudo-random number
// The seed is updated with the returned value
uint8 lfsr_char(uint8 lfsr)
{
uint8 bit;
bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 4)) & 0x01;
lfsr = (lfsr >> 1) | (bit << 7);
return lfsr;
}

int
read(int user_dst, uint64 dst, int n)
{
  uint target;
  uint8 c;
  char cbuf;

  target = n;
  acquire(&rand.lock);
  while(n > 0){

    c = lfsr_char(rand.lfsr);
    rand.lfsr = c;

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  release(&rand.lock);

  return target - n;
}

int
write(int user_src, uint64 src, int n)
{
  int r = -1;

  if (n == 1){
    uint8 c;
    if(either_copyin(&c, user_src, src, 1) != -1){
        acquire(&rand.lock);
        rand.lfsr = c;
        release(&rand.lock);
        r = 1;
    }
  }
  return r;
}

void
randinit(void)
{
  initlock(&rand.lock, "rand");

  rand.lfsr = INIT_SEED;

  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[RANDOM].read = read;
  devsw[RANDOM].write = write;
}