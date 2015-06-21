/*
 * Copyright (c) 2015 Sergi Granell (xerpi)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"
#include "vita_module.h"

static void usage();

int main(int argc, char *argv[])
{
	uint32_t i;
	uint32_t cnt;

	if (argc < 2) {
		usage();
		goto exit_error;
	}

	FILE *fp = fopen(argv[1], "rb");

	Elf32_Ehdr ehdr;
	if (!elf_read_ehdr(fp, &ehdr)) {
		fprintf(stderr, "ERROR: loading ELF\n");
		goto exit_close;
	}

	Elf32_Phdr *phdr;
	if (!elf_load_phdr(fp, &ehdr, &phdr)) {
		fprintf(stderr, "ERROR: loading ELF Program Header\n");
		goto exit_close;
	}

	Elf32_Shdr *shdr;
	if (!elf_load_shdr(fp, &ehdr, &shdr)) {
		fprintf(stderr, "ERROR: loading ELF Section Header\n");
		goto exit_free_phdr;
	}

	elf_print_ehdr(&ehdr);

	uint32_t modinfo_seg = ehdr.e_entry >> 30;
	uint32_t modinfo_off = ehdr.e_entry & 0x3FFFFFFF;

	for (i = 0; i < ehdr.e_phnum; i++) {
		printf("Program Header 0x%X:\n", i);
		elf_print_phdr(&phdr[i]);
	}

	printf("Segment containing sce_module_info: %d, offset: 0x%X\n\n",
		modinfo_seg, modinfo_off);

	sce_module_info modinfo;
	fseek(fp, phdr[modinfo_seg].p_offset + modinfo_off, SEEK_SET);
	fread(&modinfo, 1, sizeof(modinfo), fp);

	sce_print_module_info(&modinfo);

	uint32_t exports_base = phdr[modinfo_seg].p_offset + modinfo.export_top;
	uint32_t exports_end = phdr[modinfo_seg].p_offset + modinfo.export_end;

	printf("Exports base: 0x%X\nExports end: 0x%X\n\n", exports_base, exports_end);

	for (i = exports_base, cnt = 0; i < exports_end; i += sizeof(sce_module_exports), cnt++) {
		sce_module_exports modexp;
		fseek(fp, i, SEEK_SET);
		fread(&modexp, 1, sizeof(modexp), fp);
		printf("sce_module_exports 0x%X:\n", cnt);
		sce_print_module_exports(&modexp);
	}

	elf_free_shdr(&shdr);
	elf_free_phdr(&phdr);
	fclose(fp);
	return EXIT_SUCCESS;

//exit_free_shdr:
	elf_free_shdr(&shdr);
exit_free_phdr:
	elf_free_phdr(&phdr);
exit_close:
	fclose(fp);
exit_error:
	return EXIT_FAILURE;
}

void usage()
{
	fprintf(stderr,
		"vitareadelf by xerpi\n"
		"Usage:\n"
		"\t./vitareadelf sample.elf\n\n");
}
