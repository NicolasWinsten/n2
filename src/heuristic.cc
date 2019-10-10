// Copyright 2017 Kakao Corp. <http://www.kakaocorp.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "n2/heuristic.h"

#include <xmmintrin.h>

#include <vector>

namespace n2 {

using std::priority_queue
using std::vector;

BaseNeighborSelectingPolicies::~BaseNeighborSelectingPolicies() {}

void NaiveNeighborSelectingPolicies::Select(const size_t m, size_t dim, distance_function dist_func,
                                            priority_queue<FurtherFirst>& result) {
    while (result.size() > m) {
        result.pop();
    }
}

void HeuristicNeighborSelectingPolicies::Select(const size_t m, size_t dim, distance_function dist_func,
                                                priority_queue<FurtherFirst>& result) {
    if (result.size() <= m) return;
   
    vector<FurtherFirst> neighbors, picked;
    // TODO(gony): minheap / priority_queue 를 사용하고 있는 부분 하나의 자료 구조를 사용할지 테스트
    MinHeap<float, HnswNode*> skipped;
    while(!result.empty()) {
        neighbors.push_back(result.top());
        result.pop();
    }

    for (size_t i = 0; i < neighbors.size(); ++i) {
        _mm_prefetch(neighbors[i].GetNode()->GetDataAsCharArray(), _MM_HINT_T0);
    }

    for (size_t i = neighbors.size() - 1; i >= 0; --i) {
        bool skip = false;
        float cur_dist = neighbors[i].GetDistance();
        const float* q = neighbors[i].GetNode()->GetDataAsFloatArray();
        for (size_t j = 0; j < picked.size(); ++j) {
            if (j < picked.size() - 1) {
                _mm_prefetch(picked[j+1].GetNode()->GetDataAsCharArray(), _MM_HINT_T0);
            }
            _mm_prefetch(q, _MM_HINT_T1);
            if (dist_func(q, picked[j].GetNode()->GetDataAsFloatArray(), dim) < cur_dist) {
                skip = true;
                break;
            }
        }

        if (!skip) {
            picked.push_back(neighbors[i]);
        } else if (save_remains_) {
            skipped.push(cur_dist, neighbors[i].GetNode());
        }
            
        if (picked.size() == m) 
            break;
    }

    for (size_t i = 0; i < picked.size(); ++i) {
        result.emplace(picked[i]);
    }

    if (save_remains_) {
        while (result.size() < m && skipped.size()) {
            result.emplace(skipped.top().data, skipped.top().key);
            skipped.pop();
        }
    }   
}

} // namespace n2
