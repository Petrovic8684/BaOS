#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#define SEGMENT_SIZE 4096U

void segmentation_init(void);
void expand_kernel_segment(unsigned int end_addr);
int expand_user_segment(unsigned int logical_start, unsigned int size);
void reset_user_segment(void);
int is_user_address(unsigned int logical_addr);
unsigned int user_logical_to_phys(unsigned int logical_addr);
void *user_ptr(void *user_addr);
const char *user_cstr(const char *user_addr);
void user_copy_in(void *kernel_dst, const void *user_src, unsigned int size);
void user_copy_out(void *user_dst, const void *kernel_src, unsigned int size);
void segmentation_track_user_region(unsigned int logical_start, unsigned int size);

#endif
