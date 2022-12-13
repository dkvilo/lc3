//
// ClangFormat: Mozilla
//

#define DK_VM_IMPLEMENTATION
#include "dk_vm.h"

#include <signal.h>
#include <stdlib.h>

int
main(int argc, const char* argv[])
{
  if (argc < 2) {
    printf("lc3 [image] ...\n");
    exit(2);
  }

  for (int j = 1; j < argc; ++j) {
    if (!dk_read_image(argv[j])) {
      printf("failed to load image: %s\n", argv[j]);
      exit(1);
    }
  }

  signal(SIGINT, dk_handle_interrupt);
  dk_vm_running = 1;

  while (dk_vm_running) {
    dk_vm_cycle();
  }

  printf("\n\n");
  return 0;
}
