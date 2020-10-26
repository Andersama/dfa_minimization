#pragma once
#include <algorithm>
#include <vector>
#include <cstdint>

namespace cmp_dfa {
    struct cmp_transition {
        uint32_t input;
        uint32_t to;
    };

    enum class cmp_flags : uint8_t {
        is_accepting         = 0x1,
        is_rejecting         = 0x2,
        is_reachable         = 0x4,
        is_equivalence_class = 0x8
    };

    struct cmp_state {
        uint32_t state_id;
        uint32_t default_to; // given any other arbitrary input, this is the state this would go to
        std::vector<cmp_transition> transitions;
        uint8_t                     state_flags;
        cmp_state() = default;
        cmp_state(uint32_t id, uint32_t to, uint8_t flags, std::vector<cmp_transition> &trs)
            : state_id{id}, default_to{to}, transitions{trs}, state_flags{flags} {};
    };

    std::vector<cmp_state> minimized_table(const std::vector<cmp_state> &states, uint32_t start_idx) {
        std::vector<cmp_state> new_states = states;
        if (!states.size())
            return new_states;
        start_idx = (start_idx < states.size()) ? start_idx : (states.size() - 1);

        std::vector<uint32_t> unique_inputs;
        std::vector<uint32_t> unique_states;
        unique_states.reserve(states.size());
        //prep work for processing reachable states
        std::swap(new_states[0], new_states[start_idx]); // swap to the front
        std::vector<uint32_t> visit;
        visit.reserve(states.size());
        unique_states.emplace_back(new_states[0].state_id);
        visit.emplace_back(0ULL);
        //for every (reachable) index
        for (size_t v = 0; v < visit.size(); v++) {
            cmp_state &nst = new_states[visit[v]];
            // we are reachable if we're in this loop
            nst.state_flags |= (uint8_t)cmp_flags::is_reachable;
            // add the default transition to state
            uint32_t prev_default = nst.default_to;
            auto     it2          = std::find(unique_states.begin(), unique_states.end(), nst.default_to);
            if (it2 == unique_states.end()) {
                unique_states.emplace_back(prev_default);
                auto nit = std::find_if(
                    new_states.begin(), new_states.end(),
                    [prev_default](const cmp_state &value) { return value.state_id == prev_default; });
                uint32_t idx = std::distance(new_states.begin(), nit);
                //already know we're unique, no need to search if idx is in visit*
                //just need to validate we found a state
                if (idx < new_states.size())
                    visit.emplace_back(idx);
            }
            // add each transition state
            for (size_t t = 0; t < nst.transitions.size(); t++) {
                cmp_transition &ntr        = nst.transitions[t];
                uint32_t        prev_input = ntr.input;
                auto            it         = std::find(unique_inputs.begin(), unique_inputs.end(), ntr.input);
                if (it == unique_inputs.end()) {
                    unique_inputs.emplace_back(prev_input);
                }
                //
                uint32_t prev_state = ntr.to;
                auto     it2        = std::find(unique_states.begin(), unique_states.end(), ntr.to);
                if (it2 == unique_states.end()) {
                    unique_states.emplace_back(prev_state);
                    auto nit = std::find_if(
                        new_states.begin(), new_states.end(),
                        [prev_state](const cmp_state &value) { return value.state_id == prev_state; });
                    uint32_t idx = std::distance(new_states.begin(), nit);
                    // already know we're unique, no need to search if idx is in visit*
                    // just need to validate we found a state
                    if (idx < new_states.size())
                        visit.emplace_back(idx);
                }
            }
        }

        std::vector<std::vector<uint32_t>> equiv_classes(2, std::vector<uint32_t>{});
        // order the visited states by index
        std::sort(visit.begin(), visit.end());
        for (size_t i = 0; i < visit.size(); i++) {
            new_states[i] = new_states[visit[i]]; // move things into place
        }
        new_states.erase(new_states.begin() + visit.size(), new_states.end()); // erase non-visited

        // reserve enough to partition all the states and an additional error state
        std::vector<uint32_t> partition_nums(new_states.size() + 1);
        // create the first obvious partition (between final / non-final states)
        for (size_t i = 0; i < new_states.size(); i++) {
            uint32_t part_0   = (bool)(new_states[i].state_flags &
                                     ((uint8_t)cmp_flags::is_accepting | (uint8_t)cmp_flags::is_rejecting));
            partition_nums[i] = part_0;
            //i is the new state id*, and its index into the array
            equiv_classes[part_0].emplace_back(i);
            //equiv_classes[part_0].emplace_back(new_states[i].state_id);
        }
        // partition the error state among the rejecting states
        partition_nums[new_states.size()] = 1;

        std::vector<uint32_t> tt_s(new_states.size() * unique_inputs.size());
        for (size_t y = 0; y < new_states.size(); y++) {
            cmp_state &st = new_states[y];
            for (size_t x = 0; x < unique_inputs.size(); x++) {
                // std::pair<uint32_t, uint32_t> &p  = trs[y * unique_inputs.size() + x];
                uint32_t &ui = tt_s[y * unique_inputs.size() + x];
                auto      it = std::find_if(st.transitions.begin(), st.transitions.end(),
                                       [x](const cmp_transition &tr) { return tr.input == x; });
                if (it == st.transitions.end()) {
                    ui = st.default_to;
                } else {
                    ui = it->to;
                }
            }
        }
        //continue until no new partitions are made (partitions = states in our new state machine)
        bool changed = true;
        while (changed) {
            changed = false;
            // for each equiv class
            for (size_t i = 0; i < equiv_classes.size(); i++) {
                // for each state supposed to be in equiv class
                // (first index is treated as the state representing the equiv class)
                uint32_t equiv_state = equiv_classes[i][0];
                uint32_t equiv_idx   = equiv_state * unique_inputs.size();
                // index of a new parition (if needed)
                size_t target_partition = equiv_classes.size();
                // check if the other states belongs in same partition
                for (size_t j = 1; j < equiv_classes[i].size();) {
                    uint32_t   test_state = equiv_classes[i][j];
                    uint32_t   test_idx   = test_state * unique_inputs.size();

                    bool next = 1;
                    for (size_t t = 0; t < unique_inputs.size(); t++) {
                        auto lhs = tt_s[equiv_idx + t];
                        auto rhs = tt_s[test_idx + t];
                        if (partition_nums[lhs] != partition_nums[rhs]) {
                            changed = true;
                            if (target_partition >= equiv_classes.size())
                                equiv_classes.emplace_back();
                            // mark the partition this state now belongs
                            partition_nums[test_state] = target_partition;
                            // move the thing from one class to the other
                            equiv_classes[i].erase(equiv_classes[i].begin() + j);
                            equiv_classes[target_partition].emplace_back(test_state);
                            next = 0;
                        }
                    }
                    j += next;
                }
            }
        }
        // edit the states
        for (size_t i = 0; i < new_states.size(); i++) {
            cmp_state &nw_state = new_states[i];
            //unmark equiv_class
            nw_state.state_flags &= ~((uint8_t)cmp_flags::is_equivalence_class);
            for (size_t e = 0; e < equiv_classes.size(); e++) {
                if (i == equiv_classes[e][0]) {
                    //mark equiv_class
                    nw_state.state_flags |= (uint8_t)cmp_flags::is_equivalence_class;
                    break;
                }
            }
            nw_state.state_id   = partition_nums[nw_state.state_id];
            nw_state.default_to = partition_nums[nw_state.default_to];
            for (size_t t = 0; t < nw_state.transitions.size(); t++) {
                nw_state.transitions[t].to = partition_nums[nw_state.transitions[t].to];
            }
        }
        /*
        for (size_t i = 0; i < equiv_classes.size(); i++) {
            cmp_state &nw_state = new_states[equiv_classes[i]];
            nw_state.state_id   = partition_nums[nw_state.state_id];
            nw_state.default_to = partition_nums[nw_state.default_to];
            for (size_t t = 0; t < nw_state.transitions.size(); t++) {
                nw_state.transitions[t].to = partition_nums[nw_state.transitions[t].to];
            }
        }
        */
        // delete states that are duplicates of an equivalence class
        auto it = std::remove_if(new_states.begin(), new_states.end(), [](const cmp_state &st) {
            return !(st.state_flags & (uint8_t)cmp_flags::is_equivalence_class);
        });
        new_states.erase(it, new_states.end());

        return new_states;
    }
}