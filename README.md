# dfa_minimization
This toy~ish header implements a dfa minimization function

For example, below we have a dfa state machine where states 0 and 2 are equivalent. Although 4 has the same transitions it is different because it is an accepting state, the algorithm works by partitioning differently behaving states (after having removed dead / unreachable ones).
```c
    std::vector<cmp_lexer::cmp_state> states(5, cmp_lexer::cmp_state{});
    for (size_t i = 0; i < states.size(); i++) {
        states[i].state_id   = i;
        states[i].default_to = states.size();
        //add two transitions
        states[i].transitions.emplace_back();
        states[i].transitions.emplace_back();
    }
    states[0].transitions     = {{0, 1}, {1, 2}};
    states[1].transitions     = {{0, 1}, {1, 3}};
    states[2].transitions     = {{0, 1}, {1, 2}};
    states[3].transitions     = {{0, 1}, {1, 4}};
    states[4].transitions     = {{0, 1}, {1, 2}};
    states.back().state_flags = (uint8_t)cmp_lexer::cmp_flags::is_accepting;

    std::vector<cmp_lexer::cmp_state> min_states = cmp_lexer::minimized_table(states, 0);
    //print the old unoptimized table
    for (size_t i = 0; i < states.size(); i++) {
        fmt::print("[{}]: "sv, states[i].state_id);
        for (size_t t = 0; t < states[i].transitions.size(); t++) {
            fmt::print("[given:{}, goto:{}] "sv, states[i].transitions[t].input, states[i].transitions[t].to);
        }
        if (states[i].state_flags & (uint8_t)cmp_lexer::cmp_flags::is_accepting)
            fmt::print("-> accepting"sv);
        if (states[i].state_flags & (uint8_t)cmp_lexer::cmp_flags::is_rejecting) //?
            fmt::print("-> rejecting"sv);
        fmt::print("\n"sv);
    }
    fmt::print("\n\n"sv);
    //print the new optimized table
    for (size_t i = 0; i < min_states.size(); i++) {
        fmt::print("[{}]: "sv, min_states[i].state_id);
        for (size_t t = 0; t < min_states[i].transitions.size(); t++) {
            fmt::print("[given:{}, goto:{}] "sv, min_states[i].transitions[t].input, min_states[i].transitions[t].to);
        }
        if (min_states[i].state_flags & (uint8_t)cmp_lexer::cmp_flags::is_accepting)
            fmt::print("-> accepting"sv);
        if (min_states[i].state_flags & (uint8_t)cmp_lexer::cmp_flags::is_rejecting) //?
            fmt::print("-> rejecting"sv);
        fmt::print("\n"sv);
    }
```
The result is printed as below:
```
[0]: [given:0, goto:1] [given:1, goto:2]
[1]: [given:0, goto:1] [given:1, goto:3]
[2]: [given:0, goto:1] [given:1, goto:2]
[3]: [given:0, goto:1] [given:1, goto:4]
[4]: [given:0, goto:1] [given:1, goto:2] -> accepting


[0]: [given:0, goto:3] [given:1, goto:0]
[3]: [given:0, goto:3] [given:1, goto:2]
[2]: [given:0, goto:3] [given:1, goto:1]
[1]: [given:0, goto:3] [given:1, goto:0] -> accepting
```
