/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "output_handler.h"
#include "tensorflow/lite/micro/micro_log.h"

void HandleOutput(float x_value, float y_value, float t) {
  // Log the current X and Y values
  MicroPrintf("x_value: %f, y_value: %f, time: %f", static_cast<double>(x_value),
              static_cast<double>(y_value),static_cast<double>(t));
}
