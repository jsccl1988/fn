// Copyright 2014, The Project fn Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may
// not use this file except in compliance with the License. You may obtain
// a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.

#include <cstdio>

#include <map>
#include <string>
#include <vector>

#include "fn/fn.h"

using fn::_;

void word_count() {
  std::vector<std::string> words {"map", "fold", "filter", "reduce", "any"};
  auto len_count =
      _(&words).map([](const std::string& w) { return w.size(); })
               .fold_left(std::map<int, int>(), [](std::map<int, int>&& cnt,
                                                   size_t len) {
                                                     cnt[len]++;
                                                     return cnt;
                                                   });

  for (const auto& e : len_count) {
    printf("We have %d word(s) of length %d\n", e.second, e.first);
  }
}

int main() {
  word_count();
  return 0;
}

