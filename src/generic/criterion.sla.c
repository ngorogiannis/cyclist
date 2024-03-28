#include "types.c"
#include "criterion.soundness.hpp"
#include "criterion.automata_based.c"
#include "heighted_graph.hpp"

class SlaCriterion : public AutomataBasedCriterion
{

public:
    SlaCriterion(Heighted_graph *hg) : AutomataBasedCriterion(hg)
    {
    }

    bool automata_check()
    {
        auto start =  std::chrono::system_clock::now();
        bool result = this->hg->sla_automata_check();
        auto end = std::chrono::system_clock::now();
        auto duration = end - start;
        printf("sla took %dus\n", duration);
        return result;
    }
};