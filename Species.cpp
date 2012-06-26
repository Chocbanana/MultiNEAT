/////////////////////////////////////////////////////////////////
// NSNEAT
// --------------------------------------------------------------
// NeuroEvolution of Augmenting Topologies C++ implementation
//
// (c) Copyright 2008, NEAT Sciences Ltd.
//
// Peter Chervenski
////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// File:        Species.cpp
// Description: Implementation of the Species class.
///////////////////////////////////////////////////////////////////////////////



#include <algorithm>

#include "Genome.h"
#include "Species.h"
#include "Random.h"
#include "Population.h"
#include "Utils.h"
#include "Parameters.h"
#include "assert.h"


namespace NEAT
{

// initializes a species with a leader genome and an ID number
Species::Species(const Genome& a_Genome, int a_ID)
{
    m_ID     = a_ID;

    // copy the initializing genome locally.
    // it is now the representative of the species.
    m_Representative = a_Genome;
    m_BestGenome = a_Genome;

    // add the first and only one individual
    m_Individuals.push_back(a_Genome);

    m_Age = 0;
    m_GensNoImprovement = 0;
    m_OffspringRqd = 0;
    m_BestFitness = a_Genome.GetFitness();
    m_BestSpecies = true;

    // Choose a random color
    m_R = static_cast<int>(RandFloat() * 255);
    m_G = static_cast<int>(RandFloat() * 255);
    m_B = static_cast<int>(RandFloat() * 255);
}

Species& Species::operator=(const Species& a_S)
{
    // self assignment guard
    if (this != &a_S)
    {
        m_ID					= a_S.m_ID;
        m_Representative        = a_S.m_Representative;
        m_BestGenome            = a_S.m_BestGenome;
        m_BestSpecies			= a_S.m_BestSpecies;
        m_WorstSpecies			= a_S.m_WorstSpecies;
        m_BestFitness			= a_S.m_BestFitness;
        m_GensNoImprovement		= a_S.m_GensNoImprovement;
        m_Age					= a_S.m_Age;
        m_OffspringRqd			= a_S.m_OffspringRqd;
        m_R						= a_S.m_R;
        m_G						= a_S.m_G;
        m_B						= a_S.m_B;

        m_Individuals = a_S.m_Individuals;
    }

    return *this;
}



// adds a new member to the species and updates variables
void Species::AddIndividual(Genome& a_Genome)
{
    m_Individuals.push_back( a_Genome );
}








// returns an individual randomly selected from the best N%
Genome Species::GetIndividual() const
{
    ASSERT(m_Individuals.size() > 0);

    // Make a pool of only evaluated individuals!
    std::vector<Genome> t_Evaluated;
    for(unsigned int i=0; i<m_Individuals.size(); i++)
    {
        if (m_Individuals[i].IsEvaluated())
            t_Evaluated.push_back( m_Individuals[i] );
    }

    ASSERT(t_Evaluated.size() > 0);

    if (t_Evaluated.size() == 1)
    {
        return (t_Evaluated[0]);
    }
    else if (t_Evaluated.size() == 2)
    {
        return (t_Evaluated[ Rounded(RandFloat()) ]);
    }

    // Roulette wheel variables init
    double t_marble = 0, t_spin = 0, t_total_fitness = 0;
    for(unsigned int i=0; i<t_Evaluated.size(); i++)
        t_total_fitness += t_Evaluated[i].GetFitness();
    t_marble = RandFloat() * t_total_fitness;


    // Warning!!!! The individuals must be sorted by best fitness for this to work
    int t_chosen_one=0;

    // Here might be introduced better selection scheme, but this works OK for now
    if (!GlobalParameters.RouletteWheelSelection)
    {
        int t_num_parents = static_cast<int>( floor((GlobalParameters.SurvivalRate * (static_cast<double>(t_Evaluated.size())))+1.0));
        ASSERT(t_num_parents>0);
        t_chosen_one = RandInt(0, t_num_parents);
    }
    else
    {
        // roulette wheel selection
        t_spin = t_Evaluated[t_chosen_one].GetFitness();
        while(t_spin < t_marble)
        {
            t_chosen_one++;
            t_spin += t_Evaluated[t_chosen_one].GetFitness();
        }
    }

    return (t_Evaluated[t_chosen_one]);
}


// returns a completely random individual
Genome Species::GetRandomIndividual() const
{
    if (m_Individuals.size() == 0) // no members yet, return representative
    {
        return m_Representative;
    }
    else
    {
        int t_rand_choice = 0;
        t_rand_choice = RandInt(0, static_cast<int>(m_Individuals.size()-1));
        return (m_Individuals[t_rand_choice]);
    }
}

// returns the leader (the member having the best fitness)
Genome Species::GetLeader() const
{
    // Don't store the leader any more
    // Perform a search over the members and return the most fit member

    // if empty, return representative
    if (m_Individuals.size() == 0)
    {
        return m_Representative;
    }

    double t_max_fitness = -99999999;
    int t_leader_idx = -1;
    for(unsigned int i=0; i<m_Individuals.size(); i++)
    {
        double t_f = m_Individuals[i].GetFitness();
        if (t_max_fitness < t_f)
        {
            t_max_fitness = t_f;
            t_leader_idx = i;
        }
    }

    ASSERT(t_leader_idx != -1);
    return (m_Individuals[t_leader_idx]);
}


Genome Species::GetRepresentative() const
{
    return m_Representative;
}

// calculates how many offspring this species should spawn
void Species::CountOffspring()
{
    m_OffspringRqd = 0;

    for(unsigned int i=0; i<m_Individuals.size(); i++)
    {
        m_OffspringRqd += m_Individuals[i].GetOffspringAmount();
    }
}


// this method performs fitness sharing
// it also boosts the fitness of the young and penalizes old species
void Species::AdjustFitness()
{
    ASSERT(m_Individuals.size() > 0);

    // iterate through the members
    for(unsigned int i=0; i<m_Individuals.size(); i++)
    {
        double t_fitness = m_Individuals[i].GetFitness();

        // the fitness must be positive
        ASSERT(t_fitness >= 0);

        // this prevents the fitness to be below zero
        if (t_fitness <= 0) t_fitness = 0.0001;

        // update the best fitness and stagnation counter
        if (t_fitness > m_BestFitness)
        {
            m_BestFitness = t_fitness;
            m_GensNoImprovement = 0;
        }

        // boost the fitness up to some young age
        if (m_Age < GlobalParameters.YoungAgeTreshold)
        {
            t_fitness *= GlobalParameters.YoungAgeFitnessBoost;
        }

        // penalty for old species
        if (m_Age > GlobalParameters.OldAgeTreshold)
        {
            t_fitness *= GlobalParameters.OldAgePenalty;
        }

        // extreme penalty if this species is stagnating for too long time
        // one exception if this is the best species found so far
        if (m_GensNoImprovement > GlobalParameters.SpeciesDropoffAge)
        {
            // the best species is always allowed to live
            if (!m_BestSpecies)
            {
                // when the fitness is lowered that much, the species will
                // likely have 0 offspring and therefore will not survive
                t_fitness *= 0.0000001;
            }
        }

        // Compute the adjusted fitness for this member
        m_Individuals[i].SetAdjFitness(t_fitness / m_Individuals.size());
    }
}







// Sorts the members of this species by fitness
bool fitness_greater(Genome* ls, Genome* rs)
{
    return ((ls->GetFitness()) > (rs->GetFitness()));
}
bool genome_greater(Genome ls, Genome rs)
{
    return (ls.GetFitness() > rs.GetFitness());
}
void Species::SortIndividuals()
{
    std::sort(m_Individuals.begin(), m_Individuals.end(), genome_greater);
}


// Removes an individual from the species by its index within the species
void Species::RemoveIndividual(unsigned int a_idx)
{
    ASSERT(a_idx < m_Individuals.size());
    m_Individuals.erase(m_Individuals.begin() + a_idx);
}


// New stuff

/*

SUMMARY OF THE EPOCH MECHANISM
--------------------------------------------------------------------------------------------------
- Adjust all species's fitness
- Count offspring per species

. Kill worst individuals for all species (delete them, not skip them!)
. Reproduce all species
. Kill the old parents

  1. Every individual in the population is a BABY before evaluation.
  2. After evaluation (i.e. lifetime), the worst individuals are killed and the others become ADULTS.
  3. Reproduction mates adults and mutates offspring.
     A mixture of BABIES and ADULTS emerges in each species.
     New species may appear in the population during the process.
  4. Then the individuals marked as ADULT are killed off.
  5. What remains is a species with the new offspring (only babies)
--------------------------------------------------------------------------------------------------

*/


// Reproduce mates & mutates the individuals of the species
// It may access the global species list in the population
// because some babies may turn out to belong in another species
// that have to be created.
// Also calls Birth() for every new baby
void Species::Reproduce(Population &a_Pop)
{
    Genome t_baby; // temp genome for reproduction

    int t_offspring_count = Rounded(GetOffspringRqd());

    // no offspring?! yikes.. dead species!
    if (t_offspring_count == 0)
    {
        // maybe do something else?
        return;
    }

    //////////////////////////
    // Reproduction

    // Spawn t_offspring_count babies
    bool t_champ_chosen = false;
    bool t_baby_is_clone = false;
    while(t_offspring_count--)
    {
        // if the champ was not chosen, do it now..
        if (!t_champ_chosen)
        {
            t_baby = m_Individuals[0];
            t_champ_chosen = true;
        }
        // or if it was, then proceed with the others
        else
        {
            do
            {
                // this tells us if the baby is a result of mating
                bool t_mated = false;

                // There must be individuals there..
                ASSERT(NumMembers() > 0);

                // for a species of size 1 we can only mutate
                // NOTE: but does it make sense since we know this is the champ?
                if (NumMembers() == 1)
                {
                    t_baby = GetRandomIndividual();
                    t_mated = false;
                }
                // else we can mate
                else
                {
                    Genome t_mom = GetRandomIndividual();

                    // choose whether to mate at all
                    // Do not allow crossover when in simplifying phase
                    if ((RandFloat() < GlobalParameters.CrossoverRate) && (a_Pop.GetSearchMode() != SIMPLIFYING))
                    {
                        // get the father
                        Genome t_dad;
                        bool t_interspecies = false;

                        // There is a probability that the father may come from another species
                        if ((RandFloat() < GlobalParameters.InterspeciesCrossoverRate) && (a_Pop.m_Species.size()>1))
                        {
                            // Find different species (random one) // !!!!!!!!!!!!!!!!!
                            int t_diffspec = RandInt(0, static_cast<int>(a_Pop.m_Species.size()-1));
                            t_dad = a_Pop.m_Species[t_diffspec].GetRandomIndividual();
                            t_interspecies = true;
                        }
                        else
                        {
                            // Mate within species
                            t_dad = GetRandomIndividual();

                            // The other parent should be a different one
                            // number of tries to find different parent
                            int t_tries = 16;
                            while(((t_mom.GetID() == t_dad.GetID()) || (t_mom.CompatibilityDistance(t_dad) == 0)) && (t_tries--))
                            {
                                t_dad = GetRandomIndividual();
                            }
                            t_interspecies = false;
                        }

                        // OK we have both mom and dad so mate them
                        // Choose randomly one of two types of crossover
                        if (RandFloat() < GlobalParameters.MultipointCrossoverRate)
                        {
                            t_baby = t_mom.Mate( t_dad, false, t_interspecies);
                        }
                        else
                        {
                            t_baby = t_mom.Mate( t_dad, true, t_interspecies);
                        }

                        t_mated = true;
                    }
                    // don't mate - reproduce the mother asexually
                    else
                    {
                        t_baby = t_mom;
                        t_mated = false;
                    }
                }

                if (t_baby.HasDeadEnds())
                {
                    std::cout << "Dead ends in baby after crossover" << std::endl;
                    //				int p;
                    //				std::cin >> p;
                }

                // Mutate the baby
                if ((!t_mated) || (RandFloat() < GlobalParameters.OverallMutationRate))
                    MutateGenome(t_baby_is_clone, a_Pop, t_baby);

                // Check if this baby is already present somewhere in the offspring
                // we don't want that
                t_baby_is_clone = false;
                for(int i=0; i<a_Pop.m_TempSpecies.size(); i++)
                {
                    for(int j=0; j<a_Pop.m_TempSpecies[i].m_Individuals.size(); j++)
                    {
                        if (
                            //(!a_Pop.m_TempSpecies[i].m_Individuals[j].IsAdult()) // don't compare with adults (optional)
                            //&&
                            (t_baby.CompatibilityDistance(a_Pop.m_TempSpecies[i].m_Individuals[j]) < 0.000001) // identical genome?
                        )

                        {
                            t_baby_is_clone = true;
                            break;
                        }
                    }
                }
            }
            while (t_baby_is_clone);
        }

        // We have a new offspring now
        // give the offspring a new ID
        t_baby.SetID(a_Pop.GetNextGenomeID());
        a_Pop.IncrementNextGenomeID();

        // sort the baby's genes
        t_baby.SortGenes();

        // clear the baby's fitness
        t_baby.SetFitness(0);
        t_baby.SetAdjFitness(0);
        t_baby.SetOffspringAmount(0);
        t_baby.Birth();

        t_baby.ResetEvaluated();

        // debug trap
        if (t_baby.NumLinks() == 0)
        {
            std::cout << "No links in baby after reproduction" << std::endl;
//			int p;
//			std::cin >> p;
        }

        if (t_baby.HasDeadEnds())
        {
            std::cout << "Dead ends in baby after reproduction" << std::endl;
//			int p;
//			std::cin >> p;
        }

        //////////////////////////////////
        // put the baby to its species  //
        //////////////////////////////////

        // before Reproduce() is invoked, it is assumed that a
        // clone of the population exists with the name of m_TempSpecies
        // we will store results there.
        // after all reproduction completes, the original species will be replaced back

        bool t_found = false;
        std::vector<Species>::iterator t_cur_species = a_Pop.m_TempSpecies.begin();

        // No species yet?
        if (t_cur_species == a_Pop.m_TempSpecies.end())
        {
            // create the first species and place the baby there
            a_Pop.m_TempSpecies.push_back( Species(t_baby, a_Pop.GetNextSpeciesID()));
            a_Pop.IncrementNextSpeciesID();
        }
        else
        {
            // try to find a compatible species
            Genome t_to_compare = t_cur_species->GetRepresentative();

            t_found = false;
            while((t_cur_species != a_Pop.m_TempSpecies.end()) && (!t_found))
            {
                if (t_baby.IsCompatibleWith( t_to_compare ))
                {
                    // found a compatible species
                    t_cur_species->AddIndividual(t_baby);
                    t_found = true; // the search is over
                }
                else
                {
                    // keep searching for a matching species
                    t_cur_species++;
                    if (t_cur_species != a_Pop.m_TempSpecies.end())
                    {
                        t_to_compare = t_cur_species->GetRepresentative();
                    }
                }
            }

            // if couldn't find a match, make a new species
            if (!t_found)
            {
                a_Pop.m_TempSpecies.push_back( Species(t_baby, a_Pop.GetNextSpeciesID()));
                a_Pop.IncrementNextSpeciesID();
            }
        }
    }
}



// KillWorst eliminates the worst individuals from the species. It actually
// deletes them so the species size may shrink.
// Also calls Adult() for the remaining individuals (they are ready for mating)
void Species::KillWorst()
{
    ASSERT(m_Individuals.size() > 0);

    // make sure they are all babies
    for(unsigned int i=0; i<m_Individuals.size(); i++)
    {
        m_Individuals[i].Birth();
    }

    // sort the individuals by fitness
    SortIndividuals();

    // for now just keep the first N% as adults
    int t_num_parents = static_cast<int>( floor((GlobalParameters.SurvivalRate * (static_cast<double>(m_Individuals.size())))+1.0));
    ASSERT(t_num_parents>0);

    std::vector<Genome>::iterator t_iter = m_Individuals.begin();
    for(int i=0; i<t_num_parents; i++)
    {
        m_Individuals[i].Adult();
        t_iter++;
    }

    // now erase the rest
    m_Individuals.erase(t_iter, m_Individuals.end());
}



// KillOldParents eliminates the individuals with the Parent flag set.
// The species size may shrink.
void Species::KillOldParents()
{
    for(unsigned int i=0; i<m_Individuals.size(); i++)
    {
        if (m_Individuals[i].IsAdult())
        {
            m_Individuals.erase(m_Individuals.begin() + i);
            i--;
        }
    }
}



////////////
// Real-time code
void Species::CalculateAverageFitness()
{
    double t_total_fitness = 0;
    int t_num_individuals = 0;

    // consider individuals that were evaluated only!
    for(unsigned int i=0; i<m_Individuals.size(); i++)
    {
        if (m_Individuals[i].m_Evaluated)
        {
            t_total_fitness += m_Individuals[i].GetFitness();
            t_num_individuals++;
        }
    }

    if (t_num_individuals > 0)
        m_AverageFitness =  t_total_fitness / static_cast<double>(t_num_individuals);
    else
        m_AverageFitness = 0;
}




Genome Species::ReproduceOne(Population& a_Pop)
{
    Genome t_baby; // for storing the result

    //////////////////////////
    // Reproduction

    // Spawn only one baby
    bool t_champ_chosen = false;
    // this tells us if the baby is a result of mating
    bool t_mated = false;


    // There must be individuals there..
    ASSERT(NumMembers() > 0);

    // for a species of size 1 we can only mutate
    // NOTE: but does it make sense since we know this is the champ?
    if (NumMembers() == 1)
    {
        t_baby = GetIndividual();
        t_mated = false;
    }
    // else we can mate
    else
    {
        Genome t_mom = GetIndividual();

        // choose whether to mate at all
        // Do not allow crossover when in simplifying phase
        if ((RandFloat() < GlobalParameters.CrossoverRate) && (a_Pop.GetSearchMode() != SIMPLIFYING))
        {
            // get the father
            Genome t_dad;
            bool t_interspecies = false;

            // There is a probability that the father may come from another species
            if ((RandFloat() < GlobalParameters.InterspeciesCrossoverRate) && (a_Pop.m_Species.size()>1))
            {
                // Find different species (random one) // !!!!!!!!!!!!!!!!!
                // But the different species must have at least one evaluated individual
                int t_diffspec = 0;
                int t_giveup = 64;
                do
                {
                    t_diffspec = RandInt(0, static_cast<int>(a_Pop.m_Species.size()-1));
                }
                while ((a_Pop.m_Species[t_diffspec].m_AverageFitness == 0) && (t_giveup--));

                if (a_Pop.m_Species[t_diffspec].m_AverageFitness == 0)
                    t_dad = GetIndividual();
                else
                    t_dad = a_Pop.m_Species[t_diffspec].GetIndividual();

                t_interspecies = true;
            }
            else
            {
                // Mate within species
                t_dad = GetIndividual();

                // The other parent should be a different one
                // number of tries to find different parent
                int t_tries = 16;
                while(((t_mom.GetID() == t_dad.GetID()) || (t_mom.CompatibilityDistance(t_dad) == 0)) && (t_tries--))
                {
                    t_dad = GetIndividual();
                }
                t_interspecies = false;
            }

            // OK we have both mom and dad so mate them
            // Choose randomly one of two types of crossover
            if (RandFloat() < GlobalParameters.MultipointCrossoverRate)
            {
                t_baby = t_mom.Mate( t_dad, false, t_interspecies);
            }
            else
            {
                t_baby = t_mom.Mate( t_dad, true, t_interspecies);
            }
            t_mated = true;
        }
        // don't mate - reproduce the mother asexually
        else
        {
            t_baby = t_mom;
            t_mated = false;
        }
    }

    if (t_baby.HasDeadEnds())
    {
        std::cout << "Dead ends in baby after crossover" << std::endl;
//		int p;
//		std::cin >> p;
    }
    // OK we have the baby, so let's mutate it.
    bool t_baby_is_clone = false;

    if ((!t_mated) || (RandFloat() < GlobalParameters.OverallMutationRate))
        MutateGenome(t_baby_is_clone, a_Pop, t_baby);

    // We have a new offspring now
    // give the offspring a new ID
    t_baby.SetID(a_Pop.GetNextGenomeID());
    a_Pop.IncrementNextGenomeID();

    // sort the baby's genes
    t_baby.SortGenes();

    // clear the baby's fitness
    t_baby.SetFitness(0);
    t_baby.SetAdjFitness(0);
    t_baby.SetOffspringAmount(0);

    t_baby.Birth();
    t_baby.ResetEvaluated();

    // debug trap
    if (t_baby.NumLinks() == 0)
    {
        std::cout << "No links in baby after reproduction" << std::endl;
//		int p;
//		std::cin >> p;
    }

    if (t_baby.HasDeadEnds())
    {
        std::cout << "Dead ends in baby after reproduction" << std::endl;
//		int p;
//		std::cin >> p;
    }

    return t_baby;
}


// Small helper class
class Mutation
{
public:
    int MutationType;
    double Probability;
};
// For sorting
bool MutationBigger(Mutation ls, Mutation rs)
{
    return ls.Probability > rs.Probability;
}

// Mutates a genome
void Species::MutateGenome( bool t_baby_is_clone, Population &a_Pop, Genome &t_baby )
{
    // NEW version:
    // All mutations are mutually exclusive - can't have 2 mutations at once
    // for example a weight mutation and time constants mutation
    // or add link and add node and then weight mutation
    // We will perform roulette wheel selection to choose the type of mutation and will mutate the baby
    // This method guarantees that the baby will be mutated at least with one mutation
    enum MutationTypes {ADD_NODE, ADD_LINK, REMOVE_NODE, REMOVE_LINK, CHANGE_ACTIVATION_FUNCTION,
                        MUTATE_WEIGHTS, MUTATE_ACTIVATION_A, MUTATE_ACTIVATION_B, MUTATE_TIMECONSTS, MUTATE_BIASES
                       };
    std::vector<Mutation> Mutations;

    // Fill in the mutations we will perform and their probabilities
    Mutation tm;

    tm.MutationType = ADD_NODE;
    tm.Probability  = GlobalParameters.MutateAddNeuronProb;
    Mutations.push_back(tm);

    tm.MutationType = ADD_LINK;
    tm.Probability  = GlobalParameters.MutateAddLinkProb;
    Mutations.push_back(tm);

    tm.MutationType = REMOVE_NODE;
    tm.Probability  = GlobalParameters.MutateRemSimpleNeuronProb;
    Mutations.push_back(tm);

    tm.MutationType = REMOVE_LINK;
    tm.Probability  = GlobalParameters.MutateRemLinkProb;
    Mutations.push_back(tm);

    tm.MutationType = CHANGE_ACTIVATION_FUNCTION;
    tm.Probability  = GlobalParameters.MutateNeuronActivationTypeProb;
    Mutations.push_back(tm);

    tm.MutationType = MUTATE_WEIGHTS;
    tm.Probability  = GlobalParameters.MutateWeightsProb;
    Mutations.push_back(tm);

    tm.MutationType = MUTATE_ACTIVATION_A;
    tm.Probability  = GlobalParameters.MutateActivationAProb;
    Mutations.push_back(tm);

    tm.MutationType = MUTATE_ACTIVATION_B;
    tm.Probability  = GlobalParameters.MutateActivationBProb;
    Mutations.push_back(tm);

    tm.MutationType = MUTATE_TIMECONSTS;
    tm.Probability  = GlobalParameters.MutateNeuronTimeConstantsProb;
    Mutations.push_back(tm);

    tm.MutationType = MUTATE_BIASES;
    tm.Probability  = GlobalParameters.MutateNeuronBiasesProb;
    Mutations.push_back(tm);

    // Special consideration for phased searching - do not allow certain mutations depending on the search mode
    // also don't use additive mutations if we just want to get rid of the clones
    if ((a_Pop.GetSearchMode() == SIMPLIFYING) || t_baby_is_clone)
    {
        Mutations[0].Probability = 0; // add node
        Mutations[1].Probability = 0; // add link
    }
    if ((a_Pop.GetSearchMode() == COMPLEXIFYING) || t_baby_is_clone)
    {
        Mutations[2].Probability = 0; // rem node
        Mutations[3].Probability = 0; // rem link
    }

    // Now sort the mutations by probability
    std::sort(Mutations.begin(), Mutations.end(), MutationBigger);

    bool t_mutation_success = false;

    // Roulette wheel variables init
    double t_marble = 0, t_spin = 0, t_total_probability = 0;
    for(unsigned int i=0; i<Mutations.size(); i++)
        t_total_probability += Mutations[i].Probability;
    t_marble = RandFloat() * t_total_probability;
    int t_chosen_one=0;

    // Here might be introduced better selection scheme, but this works OK for now
    // roulette wheel selection
    t_spin = Mutations[t_chosen_one].Probability;
    while(t_spin < t_marble)
    {
        t_chosen_one++;
        t_spin += Mutations[t_chosen_one].Probability;
    }
    int ChosenMutation = Mutations[t_chosen_one].MutationType;

    // Now mutate based on the choice
    switch(ChosenMutation)
    {
    case ADD_NODE:
        t_mutation_success = t_baby.Mutate_AddNeuron(a_Pop.AccessInnovationDatabase());
        break;

    case ADD_LINK:
        t_mutation_success = t_baby.Mutate_AddLink(a_Pop.AccessInnovationDatabase());
        break;

    case REMOVE_NODE:
        t_mutation_success = t_baby.Mutate_RemoveSimpleNeuron(a_Pop.AccessInnovationDatabase());
        break;

    case REMOVE_LINK:
    {
        // Keep doing this mutation until it is sure that the baby will not
        // end up having dead ends or no links
        Genome t_saved_baby = t_baby;
        bool t_no_links = false, t_has_dead_ends = false;

        int t_tries = 128;
        do
        {
            t_tries--;
            if (t_tries <= 0)
            {
                t_saved_baby = t_baby;
                break; // give up
            }

            t_saved_baby = t_baby;
            t_mutation_success = t_saved_baby.Mutate_RemoveLink();

            t_no_links = t_has_dead_ends = false;

            if (t_saved_baby.NumLinks() == 0)
                t_no_links = true;

            t_has_dead_ends = t_saved_baby.HasDeadEnds();

        }
        while(t_no_links || t_has_dead_ends);

        t_baby = t_saved_baby;

        // debugger trap
        if (t_baby.NumLinks() == 0)
        {
            std::cout << "No links in baby after mutation" << std::endl;
            //						int p;
            //						std::cin >> p;
        }
        if (t_baby.HasDeadEnds())
        {
            std::cout << "Dead ends in baby after mutation" << std::endl;
            //						int p;
            //						std::cin >> p;
        }
    }
    break;

    case CHANGE_ACTIVATION_FUNCTION:
        t_baby.Mutate_NeuronActivation_Type();
        t_mutation_success = true;
        break;

    case MUTATE_WEIGHTS:
        t_baby.Mutate_LinkWeights();
        t_mutation_success = true;
        break;

    case MUTATE_ACTIVATION_A:
        t_baby.Mutate_NeuronActivations_A();
        t_mutation_success = true;
        break;

    case MUTATE_ACTIVATION_B:
        t_baby.Mutate_NeuronActivations_B();
        t_mutation_success = true;
        break;

    case MUTATE_TIMECONSTS:
        t_baby.Mutate_NeuronTimeConstants();
        t_mutation_success = true;
        break;

    case MUTATE_BIASES:
        t_baby.Mutate_NeuronBiases();
        t_mutation_success = true;
        break;

    default:
        t_mutation_success = false;
        break;
    }
}








} // namespace NEAT
