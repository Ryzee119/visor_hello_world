import argparse
from elftools.elf.elffile import ELFFile

def print_memory_usage(elf_path):
    with open(elf_path, 'rb') as f:
        elf = ELFFile(f)

        memory_usage = 0
        heap_size = 0
        stack_size = 0
        
        for section in elf.iter_sections():
            name = section.name
            size = section['sh_size']
            flags = section['sh_flags']
            sh_type = section['sh_type']
            #sh_link = section['sh_link']
            #sh_info = section['sh_info']

            if sh_type == 'SHT_PROGBITS':
                memory_usage += size

            if '.heap' in name:
                heap_size += size

            if '.stack' in name:
                stack_size += size

        print(f"Availble Heap Memory: {round(heap_size/1024)} kB")
        print(f"Available Stack Memory: {round(stack_size)} bytes")
        print(f"ROM Usage: {round(memory_usage/1024)} kB ({round(memory_usage/1024/256*100)}%)")

    return memory_usage

def main():
    parser = argparse.ArgumentParser(description='Calculate memory usage of an ELF file.')
    parser.add_argument('elf_file', type=str, help='Path to the ELF file')
    args = parser.parse_args()
    
    print_memory_usage(args.elf_file)

if __name__ == "__main__":
    main()
