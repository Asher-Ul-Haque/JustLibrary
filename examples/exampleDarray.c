#include <justUtils/memory/tracker.h>
#include <justUtils/dataStructures/dynamicArray.h>

int main(void)
{

  JustDynamicArray vector;

  justMemorySetLimit(sizeof(int) * 10, "TEST");
  JUST_DARRAY_INIT_TAGGED(&vector, 10, int, "TEST");
  justMemoryLogUsage(true);
}
