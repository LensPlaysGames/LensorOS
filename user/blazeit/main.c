int main(int argc, char **argv) {
  while(1) {
    __asm__("mov $0, %%rax\r\n\t"
			"int $0x80\r\n\t"
			:::"rax"
			);
  }
  return 420;
}
