#include <cstdlib>
#include "page.hpp"

int main(int, char const *[])
{
  constexpr size_t PageSize = 0x01 << 16;
  auto *data = std::malloc(PageSize);

  gmb::page<PageSize> p{data};

  auto *block1 = p.allocate(0x01 << 8);
  auto *block2 = p.allocate(0x01 << 9);
  auto *block3 = p.allocate(0x01 << 8);
  auto *block4 = p.allocate(0x01 << 12);

  p.deallocate(block3);
  p.deallocate(block2);

  block2 = p.allocate(0x01 << 13);

  p.deallocate(block1);
  p.deallocate(block2);
  p.deallocate(block4);

  std::free(data);

  return 0;
}
