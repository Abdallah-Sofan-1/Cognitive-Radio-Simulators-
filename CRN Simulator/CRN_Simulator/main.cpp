#include <iostream>
#include <algorithm>
#include <random>
#include <vector>
#include <iomanip>
#include <cmath>
#include <iterator>
#include<fstream>
#include<string>

using namespace std;

// Random Seeds
const unsigned int seed = 123;
mt19937_64 randEngine{ seed };
static mt19937 generator(12345);


//////////////////////////////////////////////Constants///////////////////////////////////

const int MAX_TIME_SLOT = 30'000;//
double P_Of_Activation = 0.2;
const int T_INACTIVE = 100;//
int T_ACTIVE = floor((T_INACTIVE * P_Of_Activation) / (1 - P_Of_Activation));
float P_ONToOFF = 1.0f / T_ACTIVE;
float P_OFFToON = 1.0f / T_INACTIVE;
vector<bool> SensingResultsDueToMajority(100);
vector<int> IndexOfEmptyBands_Majority;
vector<double> WhichP_Of_Activation = { 0.2 , 0.4 , 0.5 , 0.6 };
int NumberOfBandsForAllocation = 5;

int FalseAlarmCounter_Majority_ForOneTimeSLot = 0;
int MissDetictionCounter_Majority_ForOneTimeSLot = 0;
int FalseAlarmCounter_Majority_AllTimeSlots = 0;// must be cleared for every 30'000 slot
int MissDetictionCounter_Majority_AllTimeSlots = 0;// must be cleared for every 30'000 slot
int InterferenceCounterForOneTimeSlot = 0; // must be  cleared every time slot

vector <int> IndexOfAllAllocatedBands;// the size will be 50  // Used to calculate interference and utilization
vector<int> IndexOfOnPUs;
vector<int> IndexOfUsedBands;

double AverageFalseAlarmPerTimeSlot_Local = 0;      //must be automatically cleared:
double AverageMissDetectionPerTimeSlot_Local = 0;   // new calculation will be saved inside it automatically
double AverageFalseAlarmPerTimeSlot_Majority = 0;
double AverageMissDetectionPerTimeSlot_Majority = 0;
double AverageMissDetectionForAllTimeSlot_Majority = 0;//must be automatically cleared:
double AverageFalseAlarmForAllTimeSlot_Majority = 0;   // new calculation will be saved inside it automatically
double AverageThrouputPerTimeSLot = 0; // need to be calculated every time slot
double AverageUtilizationPerTimeSLot = 0;// need to be calculated every time slot
double AverageSuccessfulTransmissionPerTimeSLot = 0;// need to be calculated every time slot
double AverageInterferencePerTimeSlot = 0;
double AverageCollisionsPerTimeSLot = 0;

//////////////////////////////////////////////enums///////////////////////////////////

enum enActivationMethodsAndSensingMethods { Deterministic_Local = 0, Deterministic_Majortiy = 1, DTMC_Local = 2, DTMC_Majority = 3 };

vector< enActivationMethodsAndSensingMethods> ActivationMethodsAndSensingMethods =
    { enActivationMethodsAndSensingMethods::Deterministic_Local ,
     enActivationMethodsAndSensingMethods::Deterministic_Majortiy,
     enActivationMethodsAndSensingMethods::DTMC_Local ,
     enActivationMethodsAndSensingMethods::DTMC_Majority };
//////////////////////////////////////////////sturct///////////////////////////////////

struct stPU
{
    bool PU_Activity = 0;
    int	 T_Start = 0;
    int OnCounter = 0;
    int OffCounter = 0;

    bool Interference = 0; // must be cleared every time slot
    int InterferenceCounterForAllTime = 0;// must be cleared every run
    bool UtilizationForEveryTimeSlot = 0;// must be cleared every time slot
    int UtilizationCounterAllTimeSlots = 0;// must be cleared every run
    double AverageUtilizationPerBand = 0; // call at the end of program
    bool Throughput = 0;
    int ThroughputCounterForAllTime = 0;// must be cleared every run
    double AverageThorughPutPerBand = 0;// call this after T_MAX  // must be cleared or rewrite on it every run
    double AverageInterferencePerBand = 0;
};

struct stSU
{
    vector<bool> SensingResults;
    vector<int> IndexOfEmptyBands_Local;
    vector<int> IndexOfAllocatedBands;
    const float P_FALSE_ALARM = 0.1; // senses that PU is on while it is off
    const float P_MISS_DETICTION = 0.1; //senses that PU is off while it is on

    int MissDetectionCounterForOneTimeSlot = 0;
    int FalseAlarmCounterForOneTimeSlot = 0;
    int MissDetectionCounterForAllTimeSlot = 0;// must be cleared for every 30'000 slot
    int FalseAlarmCounterForAllTimeSlot = 0;// must be cleared for every 30'000 slot
    double AverageMissDetectionForAllTimeSlot = 0;
    double AverageFalseAlarmForAllTimeSlot = 0;
    int SUsCollisionsCounterForAllTime = 0;    // must be cleared for every 30'000 slot
    int SUsCollisionsCounterForOneTimeSlot = 0;
    int SuccessfulTransmissionCounterForOneTimeSlot = 0;// must be cleared every time slot
    int SuccessfulTransmissionCounterForAllTime = 0; // must be cleared every run
    double AverageSuccessfulTransmissionsPerBand = 0; // call it in the end of T_Max
    double AverageCollisionsPerBand = 0; // call it at the end of T_MAX
};

vector<stPU> PUs(100);
vector<stSU> SUs(10);

//////////////////////////////////////////////Function///////////////////////////////////

bool FlipACoin(float p)
{
    bernoulli_distribution Flip{ p };
    if (Flip(randEngine))
        return true;
    else
        return false;
}
int  RandomNumberFrom0To99()
{
    uniform_int_distribution<int> distribution(0, 99);// from 0 to 99

    return distribution(generator); // Get the "random" number
}

void Get_T_StartForAllPUs()
{
    for (short IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        PUs.at(IndexOfPU).T_Start = RandomNumberFrom0To99();
    }
}

void PUs_ActivationDeterminstic(int TimeSlot)// includes FLipping when T_start & Toggling at Ta or Tin_a
{
    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        if (PUs.at(IndexOfPU).PU_Activity == 1 && (TimeSlot > PUs.at(IndexOfPU).T_Start))
        {
            PUs.at(IndexOfPU).OnCounter++;
        }
        else if (PUs.at(IndexOfPU).PU_Activity == 0 && (TimeSlot > PUs.at(IndexOfPU).T_Start))
        {
            PUs.at(IndexOfPU).OffCounter++;
        }

        if ((PUs.at(IndexOfPU).PU_Activity == 0) && (PUs.at(IndexOfPU).OffCounter == T_INACTIVE))
        {
            PUs.at(IndexOfPU).PU_Activity = 1;// PU is Off & Reached  T_INACTIVE: Toggle
            PUs.at(IndexOfPU).OffCounter = 0;
        }

        else if ((PUs.at(IndexOfPU).PU_Activity == 1) && (PUs.at(IndexOfPU).OnCounter == T_ACTIVE))
        {
            PUs.at(IndexOfPU).PU_Activity = 0;// PU is On & Reached Ta: Toggle
            PUs.at(IndexOfPU).OnCounter = 0;
        }

        else if (TimeSlot == PUs.at(IndexOfPU).T_Start)
            PUs.at(IndexOfPU).PU_Activity = 1;// if (TimeSlot == PUs.at(PU_Index).T_Start)

        }
}
void Markov_Chain()
{

    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {

        if (PUs.at(IndexOfPU).PU_Activity == 0)
        {
            PUs.at(IndexOfPU).PU_Activity = FlipACoin(P_OFFToON);
        }

        else if (PUs.at(IndexOfPU).PU_Activity == 1)// PU is ON
        {
            PUs.at(IndexOfPU).PU_Activity = !FlipACoin(P_ONToOFF);
        }

    }

}

void CalculateAverageFalseAlarm_MissDetectionPerTimeSlot_Local()
{
    int FalseAlarmCounterForAllSUsInOneTimeSlot = 0;
    int MissDetectionCounterForAllSUsInOneTimeSlot = 0;// these two lines work as idenfire for these variables and reseter too

    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        FalseAlarmCounterForAllSUsInOneTimeSlot += SUs.at(IndexOfSU).FalseAlarmCounterForOneTimeSlot;
        MissDetectionCounterForAllSUsInOneTimeSlot += SUs.at(IndexOfSU).MissDetectionCounterForOneTimeSlot;
    }
    AverageFalseAlarmPerTimeSlot_Local = (float)FalseAlarmCounterForAllSUsInOneTimeSlot / SUs.size();
    AverageMissDetectionPerTimeSlot_Local = (float)MissDetectionCounterForAllSUsInOneTimeSlot / SUs.size();
    // must output these two every time slot

}
void CalculateAverageFalseAlarm_MissDetectionPerBand_Local()
{
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        SUs.at(IndexOfSU).AverageFalseAlarmForAllTimeSlot = (float)SUs.at(IndexOfSU).FalseAlarmCounterForAllTimeSlot / MAX_TIME_SLOT;
        SUs.at(IndexOfSU).AverageMissDetectionForAllTimeSlot = (float)SUs.at(IndexOfSU).MissDetectionCounterForAllTimeSlot / MAX_TIME_SLOT;
    }
}

void LocalSensing()
{
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        SUs.at(IndexOfSU).FalseAlarmCounterForOneTimeSlot = 0;
        SUs.at(IndexOfSU).MissDetectionCounterForOneTimeSlot = 0;
    }// to reset FalseAlarmCounterForOneTimeSlot MissDetectionCounterForOneTimeSlotevery time slot

    bool FalseAlarm;
    bool MissDetection;

    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++) // do sensing for all 10 Secondary users
    {

        for (short Band = 0; Band < PUs.size(); Band++) // Sense All Bands
        {
            if (PUs.at(Band).PU_Activity == 1)// if PU is on Check Miss detection
            {
                MissDetection = FlipACoin(SUs.at(IndexOfSU).P_MISS_DETICTION);

                SUs.at(IndexOfSU).SensingResults.at(Band) = !(MissDetection);
                if (MissDetection == 1)// For Miss detection counting
                {
                    SUs.at(IndexOfSU).MissDetectionCounterForOneTimeSlot++;
                    SUs.at(IndexOfSU).MissDetectionCounterForAllTimeSlot++;
                }
            }
            else // if (PUs.at(Band).PU_Activity == 0) // if PU is off Check False Alarm
            {
                FalseAlarm = FlipACoin(SUs.at(IndexOfSU).P_FALSE_ALARM);

                SUs.at(IndexOfSU).SensingResults.at(Band) = (FalseAlarm);
                if (FalseAlarm == 1)// For False Alarm counting
                {
                    SUs.at(IndexOfSU).FalseAlarmCounterForOneTimeSlot++;
                    SUs.at(IndexOfSU).FalseAlarmCounterForAllTimeSlot++;
                }
            }
        }

    }

    //CalculateAverageFalseAlarm_MissDetectionPerTimeSlot_Local();
}

void CountMissDetection_FalseAlarmForMajoritySensing()
{

    for (int Band = 0; Band < PUs.size(); Band++)
    {

        if ((PUs.at(Band).PU_Activity == 1) && (SensingResultsDueToMajority.at(Band) == 0))
        {
            MissDetictionCounter_Majority_ForOneTimeSLot++; // output this for question 1 and 2 every time slot
            MissDetictionCounter_Majority_AllTimeSlots++;
        }
        else if ((PUs.at(Band).PU_Activity == 0) && (SensingResultsDueToMajority.at(Band) == 1))
        {
            FalseAlarmCounter_Majority_ForOneTimeSLot++; // output this for question 1 and 2 every time slot
            FalseAlarmCounter_Majority_AllTimeSlots++;
        }

    }
}

void MajoritySensing()
{
    FalseAlarmCounter_Majority_ForOneTimeSLot = 0;// reset to zero each time slot
    MissDetictionCounter_Majority_ForOneTimeSLot = 0;// reset to zero each time slot

    LocalSensing();
    int OnSUsCounter = 0;
    for (int Band = 0; Band < PUs.size(); Band++)// for all bands do the majority rule
    {

        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)// this will check SUs sensing result for 1 Band
        {
            if (SUs.at(IndexOfSU).SensingResults.at(Band) == 1)
            {
                OnSUsCounter++;
            }
        }
        if (OnSUsCounter >= ((SUs.size()) / 2))
        {
            SensingResultsDueToMajority.at(Band) = 1;
        }
        else
            SensingResultsDueToMajority.at(Band) = 0;

        OnSUsCounter = 0;
    }
    CountMissDetection_FalseAlarmForMajoritySensing();
}

void CalculateAverageFalseAlarm_MissDetection_MajorityPerTimeSLot()
{
    AverageFalseAlarmPerTimeSlot_Majority = (float)FalseAlarmCounter_Majority_ForOneTimeSLot / 1;
    AverageMissDetectionPerTimeSlot_Majority = (float)MissDetictionCounter_Majority_ForOneTimeSLot / 1;
}
void CalculateAverageFalseAlarm_MissDetectionPerBand_Majority()
{// call it in the end of program
    AverageFalseAlarmForAllTimeSlot_Majority = (float)FalseAlarmCounter_Majority_AllTimeSlots / MAX_TIME_SLOT;
    AverageMissDetectionForAllTimeSlot_Majority = (float)MissDetictionCounter_Majority_AllTimeSlots / MAX_TIME_SLOT;
} // 3

void GetIndexOfEmptyBands_Local_Sensing()
{
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    // from SU0 to SU 9
    {
        SUs.at(IndexOfSU).IndexOfEmptyBands_Local.clear();
    } // clear IndexOfEmptyBands_Local vector every time slot

    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)// from SU0 to SU 9
    {
        for (int Band = 0; Band < PUs.size(); Band++)
        {
            if (SUs.at(IndexOfSU).SensingResults.at(Band) == 0)
                SUs.at(IndexOfSU).IndexOfEmptyBands_Local.push_back(Band);
        }
    }
}
void GetIndexOfEmptyBands_Majority_Sensing()
{
    IndexOfEmptyBands_Majority.clear();// clear IndexOfEmptyBands_Majority vector every time slot

    for (int Band = 0; Band < PUs.size(); Band++)
    {
        if (SensingResultsDueToMajority.at(Band) == 0)
            IndexOfEmptyBands_Majority.push_back(Band);

    }
}

void Allocation_LocalSensing()
{

    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); ++IndexOfSU)
    {
        SUs.at(IndexOfSU).IndexOfAllocatedBands.clear();
    }

    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {

        sample(SUs.at(IndexOfSU).IndexOfEmptyBands_Local.begin(), SUs.at(IndexOfSU).IndexOfEmptyBands_Local.end(),
               back_inserter(SUs.at(IndexOfSU).IndexOfAllocatedBands), NumberOfBandsForAllocation, generator);
    }
}
void Allocation_MajoritySensing()
{
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); ++IndexOfSU) // clear old values of IndexOfAllocatedBands_Majority vector
    {
        SUs.at(IndexOfSU).IndexOfAllocatedBands.clear();
    }

    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        sample(IndexOfEmptyBands_Majority.begin(), IndexOfEmptyBands_Majority.end(),
               back_inserter(SUs.at(IndexOfSU).IndexOfAllocatedBands), NumberOfBandsForAllocation, generator);
    }
}

void FillIndexOfAllAllocatedBandsVector()// to use it in interference calculations and collisions
{
    IndexOfAllAllocatedBands.clear();// cleare it for every time slot
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        for (int AllocatedBands = 0; AllocatedBands < SUs.at(IndexOfSU).IndexOfAllocatedBands.size(); AllocatedBands++)
        {// fill the vector with SUs allocated bands
            IndexOfAllAllocatedBands.push_back(SUs.at(IndexOfSU).IndexOfAllocatedBands.at(AllocatedBands));
        }
    }
}
void GetIndexOfOnPUs()
{ // this function will be called in GetIndexOfUsedPUs function
    IndexOfOnPUs.clear(); // Must be cleared every time slot
    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        if (PUs.at(IndexOfPU).PU_Activity == 1)
            IndexOfOnPUs.push_back(IndexOfPU); // if PU is on save its index on IndexOfPUs vector
    }
}
void GetIndexUsedBands()
{ // this function is called on collision counting function ( the new one )
    GetIndexOfOnPUs();
    //FillIndexOfAllAllocatedBandsVector()  is called in slotting function before count collision so call it here will not back with benefit to us

    IndexOfUsedBands.clear(); // Must Be cleared every time slot
    IndexOfUsedBands.insert(IndexOfUsedBands.end(), IndexOfAllAllocatedBands.begin(), IndexOfAllAllocatedBands.end());
    // now  i filled index of used bands vector with index of  all allocated bands
    IndexOfUsedBands.insert(IndexOfUsedBands.end(), IndexOfOnPUs.begin(), IndexOfOnPUs.end());
    // now i coppied index of on pus into  index used bands vector

}
void CountCollisions_SucessfulTransmission()
{
    GetIndexUsedBands();
    // now if the band is used more than one time it is collision #_#

    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {// to reset SUsCollisionsCounterForOneTimeSlot  for each SU Every TimeSLot
        SUs.at(IndexOfSU).SUsCollisionsCounterForOneTimeSlot = 0;
    }
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {// to reset SuccessfulTransmissionCounterForOneTimeSlot  for each SU Every TimeSLot
        SUs.at(IndexOfSU).SuccessfulTransmissionCounterForOneTimeSlot = 0;
    }
    int RepetitionCounter = 0;
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        for (int IndexOfAllocatedBand = 0; IndexOfAllocatedBand < SUs.at(IndexOfSU).IndexOfAllocatedBands.size(); IndexOfAllocatedBand++)
        {// to compare each element of allocated bands for one SU with [Other SUs allocated bands and ON PUs]

            RepetitionCounter = 0;

            for (int UsedBand = 0; UsedBand < IndexOfUsedBands.size(); UsedBand++) //[50 -->150]
            {// to compare the 5 allocated bands of 1 SU with the (50 + #ON_PUs) Used Bands
                if (SUs.at(IndexOfSU).IndexOfAllocatedBands.at(IndexOfAllocatedBand) == IndexOfUsedBands.at(UsedBand))//
                {// if index of allocated bands  is inside index of used bands vector
                    RepetitionCounter++;
                }
            }

            if (RepetitionCounter == 1)
            {
                SUs.at(IndexOfSU).SuccessfulTransmissionCounterForOneTimeSlot++;
                SUs.at(IndexOfSU).SuccessfulTransmissionCounterForAllTime++;
            }
            else if (RepetitionCounter >= 2) // if the band is used two times(first time will be from the SU and the second time will be from a SU or PU)
            {
                SUs.at(IndexOfSU).SUsCollisionsCounterForAllTime++;
                SUs.at(IndexOfSU).SUsCollisionsCounterForOneTimeSlot++;
            }

        }
    }
}

void CalculateAverageCollisionsPerTimeSLot()
{// call it at the end of time slot
    int CollisionsCounterForAllSUsInOneTimeSlot = 0;
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        CollisionsCounterForAllSUsInOneTimeSlot += SUs.at(IndexOfSU).SUsCollisionsCounterForOneTimeSlot;
    }// got the count of Collisions for all SUs at One Time slot

    AverageCollisionsPerTimeSLot = (float)CollisionsCounterForAllSUsInOneTimeSlot / SUs.size();
}
void CalculateAverageCollisionsPerBand()
{// call this function after the T_MAX end
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        SUs.at(IndexOfSU).AverageCollisionsPerBand = (float)SUs.at(IndexOfSU).SUsCollisionsCounterForAllTime / MAX_TIME_SLOT;
    }
}

void CalculateAverageSuccessfulTransmissionPerTimeSLot()
{
    int SuccessfulTransmissionCounterForAllSUsInOneTimeSlot = 0;
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        SuccessfulTransmissionCounterForAllSUsInOneTimeSlot += SUs.at(IndexOfSU).SuccessfulTransmissionCounterForOneTimeSlot;
    }// got the count of successful transmissions for all SUs at One Time slot

    AverageSuccessfulTransmissionPerTimeSLot = (float)SuccessfulTransmissionCounterForAllSUsInOneTimeSlot / SUs.size();
}
void CalculateAverageSuccessfulTransmissionPerBand()
{// call this function after the T_MAX end
    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        SUs.at(IndexOfSU).AverageSuccessfulTransmissionsPerBand = (float)SUs.at(IndexOfSU).SuccessfulTransmissionCounterForAllTime / MAX_TIME_SLOT;
    }
}

void Interference() // both Majority and Local
{
    InterferenceCounterForOneTimeSlot = 0;

    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {// 0 -- > 100 pu
        PUs.at(IndexOfPU).Interference = false;// cleared every time slot
        if (PUs.at(IndexOfPU).PU_Activity == 0)
        {// if PU is off no interference go to the next PU
            continue;
        }

        for (int Index = 0; Index < IndexOfAllAllocatedBands.size(); Index++)
        {// test if PU band is used by any SU or more
            if (IndexOfPU == IndexOfAllAllocatedBands.at(Index)) // PU[0] =1  [ 0 , 2 , 3 , 0 , 40 , 50
            {
                PUs.at(IndexOfPU).Interference = true;
                InterferenceCounterForOneTimeSlot++;
                PUs.at(IndexOfPU).InterferenceCounterForAllTime++;
                break;
            }

        }

    }
}
void CalculateAverageInterferencePerTimeSlot()
{
    AverageInterferencePerTimeSlot = (float)InterferenceCounterForOneTimeSlot / PUs.size();
}
void CalculateAverageInterferencePerBand()
{
    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        PUs.at(IndexOfPU).AverageInterferencePerBand = (float)PUs.at(IndexOfPU).InterferenceCounterForAllTime / MAX_TIME_SLOT;

    }
}

void Utilization()
{
    int dummy = -1;
    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        PUs.at(IndexOfPU).UtilizationForEveryTimeSlot = false; //reset for every time slot
    }


    for (int IndexOfVector = 0; IndexOfVector < IndexOfAllAllocatedBands.size(); IndexOfVector++)
    { // 0 -> 100  check IndexOfAllAllocatedBands(0)
        dummy = IndexOfAllAllocatedBands.at(IndexOfVector);// now i will check if the PU of index(allocated band at index i) is on
        if ((PUs.at(dummy).UtilizationForEveryTimeSlot == false)) //     50
        { // channel is used by SU
            PUs.at(dummy).UtilizationForEveryTimeSlot = 1;
            PUs.at(dummy).UtilizationCounterAllTimeSlots++;
        }
    }

}
void CalculateAverageUtilizationPerTimeSLot()
{
    int UtilizationCounterForAllPUsInOneTimeSlot = 0;// set to 0 every call
    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        UtilizationCounterForAllPUsInOneTimeSlot += PUs.at(IndexOfPU).UtilizationForEveryTimeSlot;
    }

    AverageUtilizationPerTimeSLot = (float)UtilizationCounterForAllPUsInOneTimeSlot / PUs.size();

}
void CalculateAverageUtilizationPerBand()
{// call this function after the T_MAX end
    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        PUs.at(IndexOfPU).AverageUtilizationPerBand = (float)PUs.at(IndexOfPU).UtilizationCounterAllTimeSlots / MAX_TIME_SLOT;
    }
}

void Throughput()
{
    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        PUs.at(IndexOfPU).Throughput = 0; // reset Throughput every time slot
    }

    int RepetitionCounter = 0;

    for (int Band = 0; Band < PUs.size(); Band++)
    {
        RepetitionCounter = 0;

        if (PUs.at(Band).PU_Activity == 1)
        {
            PUs.at(Band).Throughput = 0;
            continue;
        }

        for (int Index = 0; Index < IndexOfAllAllocatedBands.size(); Index++)
        {
            if (Band == IndexOfAllAllocatedBands.at(Index))
            {
                RepetitionCounter++;
            }
        }

        if (RepetitionCounter == 1)
        {
            PUs.at(Band).Throughput = 1;
            PUs.at(Band).ThroughputCounterForAllTime++;
        }
    }

}

void CalculateAverageThrouputPerTimeSLot()
{
    int ThorughputCounterForAllPUsInOneTimeSlot = 0;// set to 0 every call
    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        ThorughputCounterForAllPUsInOneTimeSlot += PUs.at(IndexOfPU).Throughput;
    }

    AverageThrouputPerTimeSLot = (float)ThorughputCounterForAllPUsInOneTimeSlot / PUs.size();
}
void CalculateAverageThrouputPerBand()
{// call this function after the T_MAX end
    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        PUs.at(IndexOfPU).AverageThorughPutPerBand = (float)PUs.at(IndexOfPU).ThroughputCounterForAllTime / MAX_TIME_SLOT;
    }
}

void ResetVariables()
{
    T_ACTIVE = floor((T_INACTIVE * P_Of_Activation) / (1 - P_Of_Activation));

    P_ONToOFF = 1.0f / T_ACTIVE;
    P_OFFToON = 1.0f / T_INACTIVE;
    FalseAlarmCounter_Majority_AllTimeSlots = 0;
    MissDetictionCounter_Majority_AllTimeSlots = 0;

    for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
    {
        PUs.at(IndexOfPU).OnCounter = 0;
        PUs.at(IndexOfPU).OffCounter = 0;
        PUs.at(IndexOfPU).InterferenceCounterForAllTime = 0;
        PUs.at(IndexOfPU).UtilizationCounterAllTimeSlots = 0;
        PUs.at(IndexOfPU).ThroughputCounterForAllTime = 0;
    }

    for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
    {
        SUs.at(IndexOfSU).FalseAlarmCounterForAllTimeSlot = 0;
        SUs.at(IndexOfSU).MissDetectionCounterForAllTimeSlot = 0;
        SUs.at(IndexOfSU).SUsCollisionsCounterForAllTime = 0;
        SUs.at(IndexOfSU).SuccessfulTransmissionCounterForAllTime = 0;
    }
}

void Slotting_Deterministic_Local()
{
    ResetVariables();

    ofstream OutPerSlotValues, OutPerBandValues;
    //choose OutPerSlotValues file
    OutPerSlotValues.open("0." + to_string(int(P_Of_Activation * 10)) + "_Deterministic_Local_PerSlot_Values.csv");

    for (int TimeSlot = 0; TimeSlot < MAX_TIME_SLOT; TimeSlot++)
    {
        PUs_ActivationDeterminstic(TimeSlot);

        LocalSensing();
        GetIndexOfEmptyBands_Local_Sensing();

        Allocation_LocalSensing();

        FillIndexOfAllAllocatedBandsVector();// Cleared Every Time Slot

        CalculateAverageFalseAlarm_MissDetectionPerTimeSlot_Local();// Have New Value Each Time Slot

        CountCollisions_SucessfulTransmission();// Cleared Every Time Slot

        CalculateAverageCollisionsPerTimeSLot();// Have New Value Each Time Slot

        Interference();// Cleared Every Time Slot

        CalculateAverageInterferencePerTimeSlot();// Have New Value Each Time Slot

        Utilization();// Cleared Every Time Slot

        CalculateAverageUtilizationPerTimeSLot();// Have New Value Each Time Slot

        Throughput();// Cleared Every Time Slot

        CalculateAverageThrouputPerTimeSLot();// Have New Value Each Time Slot

        CalculateAverageSuccessfulTransmissionPerTimeSLot();// Have New Value Each Time Slot

        // Output Per Slot Values
        {

            OutPerSlotValues
                << PUs.at(27).Throughput << "," << AverageThrouputPerTimeSLot << ","
                << PUs.at(27).UtilizationForEveryTimeSlot << "," << AverageUtilizationPerTimeSLot << ","
                << SUs.at(7).SuccessfulTransmissionCounterForOneTimeSlot << "," << AverageSuccessfulTransmissionPerTimeSLot << ","
                << SUs.at(7).SUsCollisionsCounterForOneTimeSlot << "," << AverageCollisionsPerTimeSLot << ","
                << SUs.at(7).FalseAlarmCounterForOneTimeSlot << "," << AverageFalseAlarmPerTimeSlot_Local << ","
                << SUs.at(7).MissDetectionCounterForOneTimeSlot << "," << AverageMissDetectionPerTimeSlot_Local << ","
                << PUs.at(27).Interference << "," << AverageInterferencePerTimeSlot << endl;
        }
    }
    OutPerSlotValues.close();

    CalculateAverageFalseAlarm_MissDetectionPerBand_Local();
    CalculateAverageCollisionsPerBand();
    CalculateAverageInterferencePerBand();
    CalculateAverageUtilizationPerBand();
    CalculateAverageThrouputPerBand();
    CalculateAverageSuccessfulTransmissionPerBand();

    //choose OutPerBandValues file
    OutPerBandValues.open("0." + to_string(int(P_Of_Activation * 10)) + "_Deterministic_Local_PerBand_Values.csv");

    //Output Per Band Values
    {

        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageThorughPutPerBand << ",";//first raw
        }
        OutPerBandValues << endl;


        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageUtilizationPerBand << ",";//2nd raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageSuccessfulTransmissionsPerBand << ",";//3rd raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageCollisionsPerBand << ","; //4th raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageFalseAlarmForAllTimeSlot << ",";//5th raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageMissDetectionForAllTimeSlot << ",";//6th raw
        }
        OutPerBandValues << endl;

        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageInterferencePerBand << ",";// 7th raw
        }
        OutPerBandValues << endl;
    }

    OutPerBandValues.close();
}

void Slotting_Deterministic_Majority()
{
    ResetVariables();

    ofstream OutPerSlotValues, OutPerBandValues;
    //choose OutPerSlotValues file
    OutPerSlotValues.open("0." + to_string(int(P_Of_Activation * 10)) + "_Deterministic_Majority_PerSlot_Values.csv");


    for (int TimeSlot = 0; TimeSlot < MAX_TIME_SLOT; TimeSlot++)
    {
        PUs_ActivationDeterminstic(TimeSlot);

        MajoritySensing();
        GetIndexOfEmptyBands_Majority_Sensing();

        Allocation_MajoritySensing();

        FillIndexOfAllAllocatedBandsVector();// Cleared Every Time slot

        CalculateAverageFalseAlarm_MissDetection_MajorityPerTimeSLot();// Have New Value Each Time Slot

        CountCollisions_SucessfulTransmission();// Cleared Every Time slot

        CalculateAverageCollisionsPerTimeSLot();// Have New Value Each Time Slot

        Interference();// Cleared Every Time Slot

        CalculateAverageInterferencePerTimeSlot();// Have New Value Each Time Slot

        Utilization();// Cleared Every Time Slot

        CalculateAverageUtilizationPerTimeSLot();// Have New Value Each Time Slot

        Throughput();// Cleared Every Time Slot

        CalculateAverageThrouputPerTimeSLot();// Have New Value Each Time Slot

        CalculateAverageSuccessfulTransmissionPerTimeSLot();// Have New Value Each Time Slot
        // Output Per Slot Values
        {
            OutPerSlotValues
                << PUs.at(27).Throughput << "," << AverageThrouputPerTimeSLot << ","
                << PUs.at(27).UtilizationForEveryTimeSlot << "," << AverageUtilizationPerTimeSLot << ","
                << SUs.at(7).SuccessfulTransmissionCounterForOneTimeSlot << "," << AverageSuccessfulTransmissionPerTimeSLot << ","
                << SUs.at(7).SUsCollisionsCounterForOneTimeSlot << "," << AverageCollisionsPerTimeSLot << ","
                << FalseAlarmCounter_Majority_ForOneTimeSLot << "," << AverageFalseAlarmPerTimeSlot_Majority << ","
                << MissDetictionCounter_Majority_ForOneTimeSLot << "," << AverageMissDetectionPerTimeSlot_Majority << ","
                << PUs.at(27).Interference << "," << AverageInterferencePerTimeSlot << endl;
        }
    }
    OutPerSlotValues.close();

    CalculateAverageFalseAlarm_MissDetectionPerBand_Majority();
    CalculateAverageCollisionsPerBand();
    CalculateAverageInterferencePerBand();
    CalculateAverageUtilizationPerBand();
    CalculateAverageThrouputPerBand();
    CalculateAverageSuccessfulTransmissionPerBand();

    //choose OutPerBandValues file
    OutPerBandValues.open("0." + to_string(int(P_Of_Activation * 10)) + "_Deterministic_Majority_PerBand_Values.csv");
    //Output Per Band Values
    {

        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageThorughPutPerBand << ",";//first raw
        }
        OutPerBandValues << endl;


        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageUtilizationPerBand << ",";//2nd raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageSuccessfulTransmissionsPerBand << ",";//3rd raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageCollisionsPerBand << ","; //4th raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << AverageFalseAlarmForAllTimeSlot_Majority << ",";//5th raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << AverageMissDetectionForAllTimeSlot_Majority << ",";//6th raw
        }
        OutPerBandValues << endl;

        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageInterferencePerBand << ",";// 7th raw
        }
        OutPerBandValues << endl;
    }

    OutPerBandValues.close();
}

void Slotting_DTMC_Local()
{
    ResetVariables();

    ofstream OutPerSlotValues, OutPerBandValues;
    //choose OutPerSlotValues file

    OutPerSlotValues.open("0." + to_string(int(P_Of_Activation * 10)) + "_DTMC_Local_PerSlot_Values.csv");

    for (int TimeSlot = 0; TimeSlot < MAX_TIME_SLOT; TimeSlot++)
    {
        Markov_Chain();

        LocalSensing();
        GetIndexOfEmptyBands_Local_Sensing();

        Allocation_LocalSensing();

        FillIndexOfAllAllocatedBandsVector();// Cleared Every Time slot

        CalculateAverageFalseAlarm_MissDetectionPerTimeSlot_Local();// Have new value each time slot

        CountCollisions_SucessfulTransmission();// Cleared Every Time slot

        CalculateAverageCollisionsPerTimeSLot();// Have New Value Each Time Slot

        Interference();// Cleared Every Time Slot

        CalculateAverageInterferencePerTimeSlot();// Have New Value Each Time Slot

        Utilization();// Cleared Every Time Slot

        CalculateAverageUtilizationPerTimeSLot();// Have New Value Each Time Slot

        Throughput();// Cleared Every Time Slot

        CalculateAverageThrouputPerTimeSLot();// Have New Value Each Time Slot

        CalculateAverageSuccessfulTransmissionPerTimeSLot();// Have New Value Each Time Slot
        // Output Per Slot Values
        {
            OutPerSlotValues
                << PUs.at(27).Throughput << "," << AverageThrouputPerTimeSLot << ","
                << PUs.at(27).UtilizationForEveryTimeSlot << "," << AverageUtilizationPerTimeSLot << ","
                << SUs.at(7).SuccessfulTransmissionCounterForOneTimeSlot << "," << AverageSuccessfulTransmissionPerTimeSLot << ","
                << SUs.at(7).SUsCollisionsCounterForOneTimeSlot << "," << AverageCollisionsPerTimeSLot << ","
                << SUs.at(7).FalseAlarmCounterForOneTimeSlot << "," << AverageFalseAlarmPerTimeSlot_Local << ","
                << SUs.at(7).MissDetectionCounterForOneTimeSlot << "," << AverageMissDetectionPerTimeSlot_Local << ","
                << PUs.at(27).Interference << "," << AverageInterferencePerTimeSlot << endl;
        }
    }
    OutPerSlotValues.close();

    CalculateAverageFalseAlarm_MissDetectionPerBand_Local();
    CalculateAverageCollisionsPerBand();
    CalculateAverageInterferencePerBand();
    CalculateAverageUtilizationPerBand();
    CalculateAverageThrouputPerBand();
    CalculateAverageSuccessfulTransmissionPerBand();

    //choose OutPerBandValues file
    OutPerBandValues.open("0." + to_string(int(P_Of_Activation * 10)) + "_DTMC_Local_PerBand_Values.csv");

    //Output Per Band Values
    {

        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageThorughPutPerBand << ",";//first raw
        }
        OutPerBandValues << endl;


        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageUtilizationPerBand << ",";//2nd raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageSuccessfulTransmissionsPerBand << ",";//3rd raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageCollisionsPerBand << ","; //4th raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageFalseAlarmForAllTimeSlot << ",";//5th raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageMissDetectionForAllTimeSlot << ",";//6th raw
        }
        OutPerBandValues << endl;

        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageInterferencePerBand << ",";// 7th raw
        }
        OutPerBandValues << endl;
    }

    OutPerBandValues.close();

}

void Slotting_DTMC_Majority()
{
    ResetVariables();

    ofstream OutPerSlotValues, OutPerBandValues;
    //choose OutPerSlotValues file

    OutPerSlotValues.open("0." + to_string(int(P_Of_Activation * 10)) + "_DTMC_Majority_PerSlot_Values.csv");

    for (int TimeSlot = 0; TimeSlot < MAX_TIME_SLOT; TimeSlot++)
    {
        Markov_Chain();

        MajoritySensing();
        GetIndexOfEmptyBands_Majority_Sensing();

        Allocation_MajoritySensing();

        FillIndexOfAllAllocatedBandsVector();// Cleared Every Time slot

        CalculateAverageFalseAlarm_MissDetection_MajorityPerTimeSLot();// Have New Value Each Time Slot

        CountCollisions_SucessfulTransmission();// Cleared Every Time slot

        CalculateAverageCollisionsPerTimeSLot();// Have New Value Each Time Slot

        Interference();// Cleared Every Time Slot

        CalculateAverageInterferencePerTimeSlot();// Have New Value Each Time Slot

        Utilization();// Cleared Every Time Slot

        CalculateAverageUtilizationPerTimeSLot();// Have New Value Each Time Slot

        Throughput();// Cleared Every Time Slot

        CalculateAverageThrouputPerTimeSLot();// Have New Value Each Time Slot

        CalculateAverageSuccessfulTransmissionPerTimeSLot();// Have New Value Each Time Slot
        // Output Per Slot Values
        {
            OutPerSlotValues
                << PUs.at(27).Throughput << "," << AverageThrouputPerTimeSLot << ","
                << PUs.at(27).UtilizationForEveryTimeSlot << "," << AverageUtilizationPerTimeSLot << ","
                << SUs.at(7).SuccessfulTransmissionCounterForOneTimeSlot << "," << AverageSuccessfulTransmissionPerTimeSLot << ","
                << SUs.at(7).SUsCollisionsCounterForOneTimeSlot << "," << AverageCollisionsPerTimeSLot << ","
                << FalseAlarmCounter_Majority_ForOneTimeSLot << "," << AverageFalseAlarmPerTimeSlot_Majority << ","
                << MissDetictionCounter_Majority_ForOneTimeSLot << "," << AverageMissDetectionPerTimeSlot_Majority << ","
                << PUs.at(27).Interference << "," << AverageInterferencePerTimeSlot << endl;
        }
    }
    OutPerSlotValues.close();
    CalculateAverageFalseAlarm_MissDetectionPerBand_Majority();
    CalculateAverageCollisionsPerBand();
    CalculateAverageInterferencePerBand();
    CalculateAverageUtilizationPerBand();
    CalculateAverageThrouputPerBand();
    CalculateAverageSuccessfulTransmissionPerBand();

    //choose OutPerBandValues file
    OutPerBandValues.open("0." + to_string(int(P_Of_Activation * 10)) + "_DTMC_Majority_PerBand_Values.csv");

    //Output Per Band Values
    {

        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageThorughPutPerBand << ",";//first raw
        }
        OutPerBandValues << endl;


        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageUtilizationPerBand << ",";//2nd raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageSuccessfulTransmissionsPerBand << ",";//3rd raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << SUs.at(IndexOfSU).AverageCollisionsPerBand << ","; //4th raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << AverageFalseAlarmForAllTimeSlot_Majority << ",";//5th raw
        }
        OutPerBandValues << endl;


        for (int IndexOfSU = 0; IndexOfSU < SUs.size(); IndexOfSU++)
        {
            OutPerBandValues << AverageMissDetectionForAllTimeSlot_Majority << ",";//6th raw
        }
        OutPerBandValues << endl;

        for (int IndexOfPU = 0; IndexOfPU < PUs.size(); IndexOfPU++)
        {
            OutPerBandValues << PUs.at(IndexOfPU).AverageInterferencePerBand << ",";// 7th raw
        }
        OutPerBandValues << endl;
    }

    OutPerBandValues.close();
}

void RunThePrograms()
{
    Get_T_StartForAllPUs();

    for (double& ProbabilityOfActivation : WhichP_Of_Activation) // to work for different probabilities
    {
        P_Of_Activation = ProbabilityOfActivation;

        for (enActivationMethodsAndSensingMethods& MethodOfWorking : ActivationMethodsAndSensingMethods)
        {
            if (MethodOfWorking == enActivationMethodsAndSensingMethods::Deterministic_Local)
            {
                Slotting_Deterministic_Local();
            }
            else if (MethodOfWorking == enActivationMethodsAndSensingMethods::Deterministic_Majortiy)
            {
                Slotting_Deterministic_Majority();
            }
            else if (MethodOfWorking == enActivationMethodsAndSensingMethods::DTMC_Local)
            {
                Slotting_DTMC_Local();
            }
            else  if (MethodOfWorking == enActivationMethodsAndSensingMethods::DTMC_Majority)
            {
                Slotting_DTMC_Majority();
            }
        }
    }

}

void WaitingScreen()
{

    cout << "Please Wait " << system("Color 6F") << "_0" << endl
         << "                     _      _      _      _      _      _      _      _      _      _      _   " << endl
         << "                   _( )_  _( )_  _( )_  _( )_  _( )_  _( )_  _( )_  _( )_  _( )_  _( )_  _( )_ " << endl
         << "                  (_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)" << endl
         << "                   (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_) " << endl
         << "                     _                                                                     _   " << endl
         << "                   _( )_   ______  _                            _    _         _  _      _( )_ " << endl
         << "                  (_ o _)  | ___ \\| |                          | |  | |       (_)| |    (_ o _)" << endl
         << "                   (_,_)   | |_/ /| |  ___   __ _  ___   ___   | |  | |  __ _  _ | |_    (_,_) " << endl
         << "                     _     |  __/ | | / _ \\ / _` |/ __| / _ \\  | |/\\| | / _` || || __|     _   " << endl
         << "                   _( )_   | |    | ||  __/| (_| |\\__ \\|  __/  \\  /\\  /| (_| || || |_    _( )_ " << endl
         << "                  (_ o _)  \\_|    |_| \\___| \\__,_||___/ \\___|   \\/  \\/  \\__,_||_| \\__|  (_ o _)" << endl
         << "                   (_,_)                                                                 (_,_) " << endl
         << "                     _      _      _      _      _      _      _      _      _      _      _   " << endl
         << "                   _( )_  _( )_  _( )_  _( )_  _( )_  _( )_  _( )_  _( )_  _( )_  _( )_  _( )_ " << endl
         << "                  (_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)(_ o _)" << endl
         << "                   (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_)  (_,_) " << endl;
}
void EndScreen()
{
    cout << endl << endl << endl << endl << "Finished^_^\a\n"
         << "                     _____                                                          _____ " << endl
         << "                    ( ___ )--------------------------------------------------------( ___ )" << endl
         << "                     |   |                                                          |   | " << endl
         << "                     |   | ______  _____  _   _  _____  _____  _   _  _____ ______  |   | " << endl
         << "                     |   | |  ___||_   _|| \\ | ||_   _|/  ___|| | | ||  ___||  _  \\ |   | " << endl
         << "                     |   | | |_     | |  |  \\| |  | |  \\ `--. | |_| || |__  | | | | |   | " << endl
         << "                     |   | |  _|    | |  | . ` |  | |   `--. \\|  _  ||  __| | | | | |   | " << endl
         << "                     |   | | |     _| |_ | |\\  | _| |_ /\\__/ /| | | || |___ | |/ /  |   | " << endl
         << "                     |   | \\_|     \\___/ \\_| \\_/ \\___/ \\____/ \\_| |_/\\____/ |___/   |   | " << endl
         << "                     |___|                                                          |___| " << endl
         << "                    (_____)--------------------------------------------------------(_____)\a" << endl
         << system("Color 2B") << "_0";

}

int main()
{
    for (int i = 0; i < 10; i++) // Resize (Sensing result && IndexOfEmptyBands) vector for every Secondary user
    {

        SUs.at(i).SensingResults.resize(100);
        SUs.at(i).IndexOfEmptyBands_Local.reserve(100);
        SUs.at(i).IndexOfAllocatedBands.resize(5);

    }
    IndexOfEmptyBands_Majority.reserve(100);
    IndexOfAllAllocatedBands.reserve(50);
    IndexOfOnPUs.reserve(100);
    IndexOfUsedBands.reserve(50 + 100); // SUs allocated bands and On PUs

    WaitingScreen();

    RunThePrograms();

    EndScreen();

    return 0;
}
