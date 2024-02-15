layout(local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

#define lID gl_LocalInvocationID.x
#define gID gl_GlobalInvocationID.x
#define wSize gl_WorkGroupSize.x
#define wID gl_WorkGroupID.x
#define wCnt gl_NumWorkGroups.x