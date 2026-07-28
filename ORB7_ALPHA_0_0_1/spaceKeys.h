// header for spaceKeys.cpp
// Handle all the keys for the spacemouse

// FIXED: Parameter signature updated from int* to uint8_t* to optimize memory footprint [6]
void readAllFromKeys(uint8_t* keyVals);
void setupKeys();
void evalKeys(uint8_t* keyVals, uint8_t* keyOut, uint8_t* keyState);