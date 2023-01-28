#include "globalTypes.h"
#include "gpu.h"

extern struct CONFIG GlobalConfig;

static cl_device_id * gpuDevices = NULL;
static cl_context context;
static cl_int ret;

static char * source_str;
static size_t source_size;

static cl_program program;
static cl_kernel kernel;
static cl_command_queue command_queue;

static cl_mem pinnedClMemResult, clMemResult;
static cl_mem clMemPassphrase, clMemResultMask;

void load_source();
void createDevice();
void createkernel();
uint8_t * create_clobj();
void check_error(cl_int error, int position);

uint8_t * gpuInit(void) {
    load_source();
    createDevice();
    createkernel();
    return create_clobj();
}

void gpuSolver(struct PASSPHRASE * inputBatch, uint8_t * resultBatch) {
    static size_t firstRound = 1;
    if (firstRound) {
        ret = clEnqueueWriteBuffer(
            command_queue,
            clMemResultMask,
            CL_FALSE,
            0,
            sizeof(uint8_t) * RS_ADDRESS_BYTE_SIZE,
            GlobalConfig.mask,
            0,
            NULL,
            NULL
        );
        check_error(ret, 51);
        firstRound = 0;
        for (size_t i = 0; i < GlobalConfig.gpuThreads; i++) {
            resultBatch[i] = 0;
        }
    } else {
        // read hashes
        ret = clEnqueueReadBuffer(
            command_queue,
            clMemResult,
            CL_TRUE,
            0,
            sizeof(cl_uchar) * GlobalConfig.gpuThreads,
            resultBatch,
            0,
            NULL,
            NULL
        );
        check_error(ret, 54);
    }

    ret = clEnqueueWriteBuffer(
        command_queue,
        clMemPassphrase,
        CL_FALSE,
        0,
        sizeof(struct PASSPHRASE) * GlobalConfig.gpuThreads,
        inputBatch,
        0,
        NULL,
        NULL
    );
    check_error(ret, 52);
    ret = clEnqueueNDRangeKernel(
        command_queue,
        kernel,
        1,
        NULL,
        (size_t *) &GlobalConfig.gpuThreads,
        (size_t *) &GlobalConfig.gpuWorkSize,
        0,
        NULL,
        NULL
    );
    check_error(ret, 53);
}

void load_source() {
    FILE * fp;
    fp = fopen("passphraseToId.cl3.cl", "rb");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel file 'passphraseToId.cl'.\n");
        exit(1);
    }
    fseek(fp, 0L, SEEK_END);
    source_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    source_str = (char*)malloc(source_size * sizeof(char));
    if (fread(source_str, sizeof(char), source_size, fp) != source_size) {
        fprintf(stderr, "Failed to read kernel file 'passphraseToId.cl'.\n");
        exit(1);
    }
    fclose(fp);
}

uint8_t * create_clobj(void) {
    uint8_t * resultBuf;
    pinnedClMemResult = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
        sizeof(cl_uchar) * GlobalConfig.gpuThreads,
        NULL,
        &ret
    );
    check_error(ret, 107);
    resultBuf = (cl_uchar *) clEnqueueMapBuffer(
        command_queue,
        pinnedClMemResult,
        CL_TRUE,
        CL_MAP_READ,
        0,
        sizeof(cl_uchar) * GlobalConfig.gpuThreads,
        0,
        NULL,
        NULL,
        &ret
    );
    check_error(ret, 108);

    clMemPassphrase = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(struct PASSPHRASE) * GlobalConfig.gpuThreads,
        NULL,
        &ret
    );
    check_error(ret, 107);
    clMemResult = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY,
        sizeof(cl_uchar) * GlobalConfig.gpuThreads,
        NULL,
        &ret
    );
    check_error(ret, 108);
    clMemResultMask = clCreateBuffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(cl_uchar) * RS_ADDRESS_BYTE_SIZE,
        NULL,
        &ret
    );
    check_error(ret, 109);

    ret = clSetKernelArg(kernel, 0, sizeof(clMemPassphrase), (void *) &clMemPassphrase);
    check_error(ret, 120);
    ret = clSetKernelArg(kernel, 1, sizeof(clMemResult), (void *) &clMemResult);
    check_error(ret, 130);
    ret = clSetKernelArg(kernel, 2, sizeof(clMemResultMask), (void *) &clMemResultMask);
    check_error(ret, 130);
    return resultBuf;
}

void createDevice() {
    cl_platform_id* platforms;
    cl_uint platformCount;
    cl_uint deviceCount;
    cl_uint maxComputeUnits;
    char* info;
    size_t infoSize;

    // get platform count
    ret = clGetPlatformIDs(5, NULL, &platformCount);
    check_error(ret, 140);
    if (GlobalConfig.gpuPlatform >= platformCount) {
        printf("Supplied gpu-platform does not exist.\n");
        exit(1);
    }
    printf("GPU platform %llu details:\n", PRINTF_CAST GlobalConfig.gpuPlatform);

    // get all platforms
    platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
    ret = clGetPlatformIDs(platformCount, platforms, NULL);
    check_error(ret, 141);


    // get and print platform attribute value
    ret = clGetPlatformInfo(platforms[GlobalConfig.gpuPlatform], CL_PLATFORM_NAME, 0, NULL, &infoSize);
    check_error(ret, 142);
    info = (char*) malloc(infoSize);
    ret = clGetPlatformInfo(platforms[GlobalConfig.gpuPlatform], CL_PLATFORM_NAME, infoSize, info, NULL);
    check_error(ret, 143);
    printf("    Platform name...........: %s\n", info);
    free(info);

    // get devices count for selected platform
    ret = clGetDeviceIDs(platforms[GlobalConfig.gpuPlatform], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
    check_error(ret, 144);
    if (GlobalConfig.gpuDevice >= deviceCount) {
        printf("Supplied gpu-device does not exist.\n");
        exit(1);
    }
    printf("Details for device %llu:\n", PRINTF_CAST GlobalConfig.gpuDevice);
    gpuDevices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
    ret = clGetDeviceIDs(platforms[GlobalConfig.gpuPlatform], CL_DEVICE_TYPE_ALL, deviceCount, gpuDevices, NULL);
    check_error(ret, 145);

    // get and print selected device name
    ret = clGetDeviceInfo(gpuDevices[GlobalConfig.gpuDevice], CL_DEVICE_NAME, 0, NULL, &infoSize);
    check_error(ret, 146);
    info = (char*) malloc(infoSize);
    ret = clGetDeviceInfo(gpuDevices[GlobalConfig.gpuDevice], CL_DEVICE_NAME, infoSize, info, NULL);
    check_error(ret, 147);
    printf("    Device..................: %s\n", info);
    free(info);

    // print hardware device version
    ret = clGetDeviceInfo(gpuDevices[GlobalConfig.gpuDevice], CL_DEVICE_VERSION, 0, NULL, &infoSize);
    check_error(ret, 148);
    info = (char*) malloc(infoSize);
    ret = clGetDeviceInfo(gpuDevices[GlobalConfig.gpuDevice], CL_DEVICE_VERSION, infoSize, info, NULL);
    check_error(ret, 149);
    printf("    Hardware version........: %s\n", info);
    free(info);

    // print software driver version
    ret = clGetDeviceInfo(gpuDevices[GlobalConfig.gpuDevice], CL_DRIVER_VERSION, 0, NULL, &infoSize);
    check_error(ret, 150);
    info = (char*) malloc(infoSize);
    ret = clGetDeviceInfo(gpuDevices[GlobalConfig.gpuDevice], CL_DRIVER_VERSION, infoSize, info, NULL);
    check_error(ret, 151);
    printf("    Software version........: %s\n", info);
    free(info);

    // print c version supported by compiler for device
    ret = clGetDeviceInfo(gpuDevices[GlobalConfig.gpuDevice], CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &infoSize);
    check_error(ret, 152);
    info = (char*) malloc(infoSize);
    ret = clGetDeviceInfo(gpuDevices[GlobalConfig.gpuDevice], CL_DEVICE_OPENCL_C_VERSION, infoSize, info, NULL);
    check_error(ret, 153);
    printf("    OpenCL C version........: %s\n", info);
    free(info);

    // print parallel compute units
    ret = clGetDeviceInfo(gpuDevices[GlobalConfig.gpuDevice], CL_DEVICE_MAX_COMPUTE_UNITS,
            sizeof(maxComputeUnits), &maxComputeUnits, NULL);
    check_error(ret, 154);
    printf("    Parallel compute units..: %d\n", maxComputeUnits);

    // Create context for desired device
    context = clCreateContext( NULL, 1, &gpuDevices[GlobalConfig.gpuDevice], NULL, NULL, &ret);
    check_error(ret, 142);

    // Get the device MAX_WORK_GROUP_SIZE
    ret = clGetDeviceInfo(gpuDevices[GlobalConfig.gpuDevice], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(infoSize), &infoSize, NULL);
    check_error(ret, 155);
    printf("    Max Work Group Size.....: %zu\n", infoSize);
    // Set the defaults (could not be set on argumentsParser, so setting it now)
    if (GlobalConfig.gpuWorkSize == 0) {
        GlobalConfig.gpuWorkSize = infoSize;
    }
}

void createkernel() {
    program = clCreateProgramWithSource(
        context,
        1,
        (const char **) &source_str,
        (const size_t *) &source_size,
        &ret
    );
    check_error(ret, 200);
    printf("Compiling OpenCL kernel: ");
    fflush(stdout);
    ret = clBuildProgram(
        program,
        1,
        &gpuDevices[GlobalConfig.gpuDevice],
        " -cl-std=CL3.0 ",
        NULL,
        NULL
    );
    if (ret != CL_SUCCESS) {
        printf("Failed.\n");
        size_t len = 0;
        cl_int ret2 = CL_SUCCESS;
        ret2 = clGetProgramBuildInfo(program, gpuDevices[GlobalConfig.gpuDevice], CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
        char *buffer = calloc(len, sizeof(char));
        ret2 = clGetProgramBuildInfo(program, gpuDevices[GlobalConfig.gpuDevice], CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
        printf("%s\n", buffer);
        exit(1);
    }
    check_error(ret, 210);
    printf("Done!\n");
    fflush(stdout);
    kernel = clCreateKernel(program, "process", &ret);
    check_error(ret, 220);
    fflush(stdout);
#ifdef COMPILER_MSVC
#pragma warning(suppress : 4996)
    command_queue = clCreateCommandQueue(
#else
    command_queue = clCreateCommandQueue(
#endif
        context,
        gpuDevices[GlobalConfig.gpuDevice],
        0,
        &ret
    );
    check_error(ret, 230);
}

void check_error(cl_int error, int position) {
    if (error == CL_SUCCESS) {
        return;
    }
    printf("OpenCL error %d at debug position %d:\n", error, position);
    switch (error) {
    case -1:
        printf("CL_DEVICE_NOT_FOUND    clGetDeviceIDs    if no OpenCL devices that matched device_type were found.");
        break;
    case -2:
        printf("CL_DEVICE_NOT_AVAILABLE    clCreateContext    if a device in devices is currently not available even though the device was returned by clGetDeviceIDs.");
        break;
    case -3:
        printf("CL_COMPILER_NOT _AVAILABLE    clBuildProgram    if program is created with clCreateProgramWithSource and a compiler is not available i.e. CL_DEVICE_COMPILER_AVAILABLE specified in the table of OpenCL Device Queries for clGetDeviceInfo is set to CL_FALSE.");
        break;
    case -4:
        printf("CL_MEM_OBJECT _ALLOCATION_FAILURE        if there is a failure to allocate memory for buffer object.");
        break;
    case -5:
        printf("CL_OUT_OF_RESOURCES        if there is a failure to allocate resources required by the OpenCL implementation on the device.");
        break;
    case -6:
        printf("CL_OUT_OF_HOST_MEMORY        if there is a failure to allocate resources required by the OpenCL implementation on the host.");
        break;
    case -7:
        printf("CL_PROFILING_INFO_NOT _AVAILABLE    clGetEventProfilingInfo    if the CL_QUEUE_PROFILING_ENABLE flag is not set for the command-queue, if the execution status of the command identified by event is not CL_COMPLETE or if event is a user event object.");
        break;
    case -8:
        printf("CL_MEM_COPY_OVERLAP    clEnqueueCopyBuffer, clEnqueueCopyBufferRect, clEnqueueCopyImage    if src_buffer and dst_buffer are the same buffer or subbuffer object and the source and destination regions overlap or if src_buffer and dst_buffer are different sub-buffers of the same associated buffer object and they overlap. The regions overlap if src_offset ≤ to dst_offset ≤ to src_offset + size – 1, or if dst_offset ≤ to src_offset ≤ to dst_offset + size – 1.");
        break;
    case -9:
        printf("CL_IMAGE_FORMAT _MISMATCH    clEnqueueCopyImage    if src_image and dst_image do not use the same image format.");
        break;
    case -10:
        printf("CL_IMAGE_FORMAT_NOT _SUPPORTED    clCreateImage    if the image_format is not supported.");
        break;
    case -11:
        printf("CL_BUILD_PROGRAM _FAILURE    clBuildProgram    if there is a failure to build the program executable. This error will be returned if clBuildProgram does not return until the build has completed.");
        break;
    case -12:
        printf("CL_MAP_FAILURE    clEnqueueMapBuffer, clEnqueueMapImage    if there is a failure to map the requested region into the host address space. This error cannot occur for image objects created with CL_MEM_USE_HOST_PTR or CL_MEM_ALLOC_HOST_PTR.");
        break;
    case -13:
        printf("CL_MISALIGNED_SUB _BUFFER_OFFSET        if a sub-buffer object is specified as the value for an argument that is a buffer object and the offset specified when the sub-buffer object is created is not aligned to CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with queue.");
        break;
    case -14:
        printf("CL_EXEC_STATUS_ERROR_ FOR_EVENTS_IN_WAIT_LIST        if the execution status of any of the events in event_list is a negative integer value.");
        break;
    case -15:
        printf("CL_COMPILE_PROGRAM _FAILURE    clCompileProgram    if there is a failure to compile the program source. This error will be returned if clCompileProgram does not return until the compile has completed.");
        break;
    case -16:
        printf("CL_LINKER_NOT_AVAILABLE    clLinkProgram    if a linker is not available i.e. CL_DEVICE_LINKER_AVAILABLE specified in the table of allowed values for param_name for clGetDeviceInfo is set to CL_FALSE.");
        break;
    case -17:
        printf("CL_LINK_PROGRAM_FAILURE    clLinkProgram    if there is a failure to link the compiled binaries and/or libraries.");
        break;
    case -18:
        printf("CL_DEVICE_PARTITION _FAILED    clCreateSubDevices    if the partition name is supported by the implementation but in_device could not be further partitioned.");
        break;
    case -19:
        printf("CL_KERNEL_ARG_INFO _NOT_AVAILABLE    clGetKernelArgInfo    if the argument information is not available for kernel.");
        break;
    case -30:
        printf("CL_INVALID_VALUE    clGetDeviceIDs, clCreateContext    This depends on the function: two or more coupled parameters had errors.");
        break;
    case -31:
        printf("CL_INVALID_DEVICE_TYPE    clGetDeviceIDs    if an invalid device_type is given");
        break;
    case -32:
        printf("CL_INVALID_PLATFORM    clGetDeviceIDs    if an invalid platform was given");
        break;
    case -33:
        printf("CL_INVALID_DEVICE    clCreateContext, clBuildProgram    if devices contains an invalid device or are not associated with the specified platform.");
        break;
    case -34:
        printf("CL_INVALID_CONTEXT        if context is not a valid context.");
        break;
    case -35:
        printf("CL_INVALID_QUEUE_PROPERTIES    clCreateCommandQueue    if specified command-queue-properties are valid but are not supported by the device.");
        break;
    case -36:
        printf("CL_INVALID_COMMAND_QUEUE        if command_queue is not a valid command-queue.");
        break;
    case -37:
        printf("CL_INVALID_HOST_PTR    clCreateImage, clCreateBuffer    This flag is valid only if host_ptr is not NULL. If specified, it indicates that the application wants the OpenCL implementation to allocate memory for the memory object and copy the data from memory referenced by host_ptr.CL_MEM_COPY_HOST_PTR and CL_MEM_USE_HOST_PTR are mutually exclusive.CL_MEM_COPY_HOST_PTR can be used with CL_MEM_ALLOC_HOST_PTR to initialize the contents of the cl_mem object allocated using host-accessible (e.g. PCIe) memory.");
        break;
    case -38:
        printf("CL_INVALID_MEM_OBJECT        if memobj is not a valid OpenCL memory object.");
        break;
    case -39:
        printf("CL_INVALID_IMAGE_FORMAT_DESCRIPTOR        if the OpenGL/DirectX texture internal format does not map to a supported OpenCL image format.");
        break;
    case -40:
        printf("CL_INVALID_IMAGE_SIZE        if an image object is specified as an argument value and the image dimensions (image width, height, specified or compute row and/or slice pitch) are not supported by device associated with queue.");
        break;
    case -41:
        printf("CL_INVALID_SAMPLER    clGetSamplerInfo, clReleaseSampler, clRetainSampler, clSetKernelArg    if sampler is not a valid sampler object.");
        break;
    case -42:
        printf("CL_INVALID_BINARY    clCreateProgramWithBinary, clBuildProgram    The provided binary is unfit for the selected device. if program is created with clCreateProgramWithBinary and devices listed in device_list do not have a valid program binary loaded.");
        break;
    case -43:
        printf("CL_INVALID_BUILD_OPTIONS    clBuildProgram    if the build options specified by options are invalid.");
        break;
    case -44:
        printf("CL_INVALID_PROGRAM        if program is a not a valid program object.");
        break;
    case -45:
        printf("CL_INVALID_PROGRAM_EXECUTABLE        if there is no successfully built program executable available for device associated with command_queue.");
        break;
    case -46:
        printf("CL_INVALID_KERNEL_NAME    clCreateKernel    if kernel_name is not found in program.");
        break;
    case -47:
        printf("CL_INVALID_KERNEL_DEFINITION    clCreateKernel    if the function definition for __kernel function given by kernel_name such as the number of arguments, the argument types are not the same for all devices for which the program executable has been built.");
        break;
    case -48:
        printf("CL_INVALID_KERNEL        if kernel is not a valid kernel object.");
        break;
    case -49:
        printf("CL_INVALID_ARG_INDEX    clSetKernelArg, clGetKernelArgInfo    if arg_index is not a valid argument index.");
        break;
    case -50:
        printf("CL_INVALID_ARG_VALUE    clSetKernelArg, clGetKernelArgInfo    if arg_value specified is not a valid value.");
        break;
    case -51:
        printf("CL_INVALID_ARG_SIZE    clSetKernelArg    if arg_size does not match the size of the data type for an argument that is not a memory object or if the argument is a memory object and arg_size != sizeof(cl_mem) or if arg_size is zero and the argument is declared with the __local qualifier or if the argument is a sampler and arg_size != sizeof(cl_sampler).");
        break;
    case -52:
        printf("CL_INVALID_KERNEL_ARGS        if the kernel argument values have not been specified.");
        break;
    case -53:
        printf("CL_INVALID_WORK_DIMENSION        if work_dim is not a valid value (i.e. a value between 1 and 3).");
        break;
    case -54:
        printf("CL_INVALID_WORK_GROUP_SIZE        if local_work_size is specified and number of work-items specified by global_work_size is not evenly divisable by size of work-group given by local_work_size or does not match the work-group size specified for kernel using the __attribute__ ((reqd_work_group_size(X, Y, Z))) qualifier in program source.if local_work_size is specified and the total number of work-items in the work-group computed as local_work_size[0] *… local_work_size[work_dim – 1] is greater than the value specified by CL_DEVICE_MAX_WORK_GROUP_SIZE in the table of OpenCL Device Queries for clGetDeviceInfo.if local_work_size is NULL and the __attribute__ ((reqd_work_group_size(X, Y, Z))) qualifier is used to declare the work-group size for kernel in the program source.");
        break;
    case -55:
        printf("CL_INVALID_WORK_ITEM_SIZE        if the number of work-items specified in any of local_work_size[0], … local_work_size[work_dim – 1] is greater than the corresponding values specified by CL_DEVICE_MAX_WORK_ITEM_SIZES[0], …. CL_DEVICE_MAX_WORK_ITEM_SIZES[work_dim – 1].");
        break;
    case -56:
        printf("CL_INVALID_GLOBAL_OFFSET        if the value specified in global_work_size + the corresponding values in global_work_offset for any dimensions is greater than the sizeof(size_t) for the device on which the kernel execution will be enqueued.");
        break;
    case -57:
        printf("CL_INVALID_EVENT_WAIT_LIST        if event_wait_list is NULL and num_events_in_wait_list > 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events.");
        break;
    case -58:
        printf("CL_INVALID_EVENT        if event objects specified in event_list are not valid event objects.");
        break;
    case -59:
        printf("CL_INVALID_OPERATION        if interoperability is specified by setting CL_CONTEXT_ADAPTER_D3D9_KHR, CL_CONTEXT_ADAPTER_D3D9EX_KHR or CL_CONTEXT_ADAPTER_DXVA_KHR to a non-NULL value, and interoperability with another graphics API is also specified. (only if the cl_khr_dx9_media_sharing extension is supported).");
        break;
    case -60:
        printf("CL_INVALID_GL_OBJECT        if texture is not a GL texture object whose type matches texture_target, if the specified miplevel of texture is not defined, or if the width or height of the specified miplevel is zero.");
        break;
    case -61:
        printf("CL_INVALID_BUFFER_SIZE    clCreateBuffer, clCreateSubBuffer    if size is 0.Implementations may return CL_INVALID_BUFFER_SIZE if size is greater than the CL_DEVICE_MAX_MEM_ALLOC_SIZE value specified in the table of allowed values for param_name for clGetDeviceInfo for all devices in context.");
        break;
    case -62:
        printf("CL_INVALID_MIP_LEVEL    OpenGL-functions    if miplevel is greater than zero and the OpenGL implementation does not support creating from non-zero mipmap levels.");
        break;
    case -63:
        printf("CL_INVALID_GLOBAL_WORK_SIZE        if global_work_size is NULL, or if any of the values specified in global_work_size[0], …global_work_size [work_dim – 1] are 0 or exceed the range given by the sizeof(size_t) for the device on which the kernel execution will be enqueued.");
        break;
    case -64:
        printf("CL_INVALID_PROPERTY    clCreateContext    Vague error, depends on the function");
        break;
    case -65:
        printf("CL_INVALID_IMAGE_DESCRIPTOR    clCreateImage    if values specified in image_desc are not valid or if image_desc is NULL.");
        break;
    case -66:
        printf("CL_INVALID_COMPILER_OPTIONS    clCompileProgram    if the compiler options specified by options are invalid.");
        break;
    case -67:
        printf("CL_INVALID_LINKER_OPTIONS    clLinkProgram    if the linker options specified by options are invalid.");
        break;
    case -68:
        printf("CL_INVALID_DEVICE_PARTITION_COUNT    clCreateSubDevices    if the partition name specified in properties is CL_DEVICE_PARTITION_BY_COUNTS and the number of sub-devices requested exceeds CL_DEVICE_PARTITION_MAX_SUB_DEVICES or the total number of compute units requested exceeds CL_DEVICE_PARTITION_MAX_COMPUTE_UNITS for in_device, or the number of compute units requested for one or more sub-devices is less than zero or the number of sub-devices requested exceeds CL_DEVICE_PARTITION_MAX_COMPUTE_UNITS for in_device.");
        break;
    case -69:
        printf("CL_INVALID_PIPE_SIZE    clCreatePipe    if pipe_packet_size is 0 or the pipe_packet_size exceeds CL_DEVICE_PIPE_MAX_PACKET_SIZE value for all devices in context or if pipe_max_packets is 0.");
        break;
    case -70:
        printf("CL_INVALID_DEVICE_QUEUE    clSetKernelArg    when an argument is of type queue_t when it’s not a valid device queue object.");
        break;
    default:
        printf("Unknow error. Check error code: %d.", error);
    }
    printf("\n");
    exit(1);
}
