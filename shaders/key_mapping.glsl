struct Key {
    uint partitioner;
    uint bucketer;
    uint lower1;
    uint lower2;
};


uint upper32BitMultiply(uint a, uint b) {
    // Extract 16-bit halves of input values
    uint a_low = a & 0xFFFFu;
    uint a_high = a >> 16u;
    uint b_low = b & 0xFFFFu;
    uint b_high = b >> 16u;

    // Calculate the individual products
    uint product_low = a_low * b_low;
    uint product_mid1 = a_low * b_high;
    uint product_mid2 = a_high * b_low;
    uint product_high = a_high * b_high;

    // Combine the products to get the final 64-bit result
    uint result = product_high + (product_mid1 >> 16u) + (product_mid2 >> 16u);

    // Handle carry from lower to upper bits
    result += ((product_low >> 16u) + (product_mid1 & 0xFFFFu) + (product_mid2 & 0xFFFFu)) >> 16u;

    return result;
}

#include "constants.glsl"
layout(set = 1, binding = 0) buffer fulcsB {uint fulcs[];};

uint assignBucketRelative(uint keyUpper, uint bucketsPerPartition) {
	uint inter = keyUpper * (FULCS_INTER - 1);
	uint index = upper32BitMultiply(keyUpper, (FULCS_INTER - 1));
	uint v1 = upper32BitMultiply(fulcs[index + 0], inter);
	uint v2 = upper32BitMultiply(fulcs[index + 1], 0xFFFFFFFF - inter);
    
	return (v1 + v2)>>16;
}

uint assignPartition(uint partitioner, uint partitions, uint bucketsPerPartition) {
    return upper32BitMultiply(partitioner, partitions);
}

uint assignBucketAbsolute(uint partitioner, uint bucketer, uint partitions, uint bucketsPerPartition) {
	return assignBucketRelative(bucketer, bucketsPerPartition) + assignPartition(partitioner, partitions, bucketsPerPartition) * bucketsPerPartition;
}