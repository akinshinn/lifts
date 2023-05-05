#include <cmath>
#include <iostream>
#include <list>
#include <map>

#include "Control.h"

/*
TODO
-дописать full


*/
const size_t numberOfElevators = 2;

const size_t elevatorCapacity = 6;

const size_t maxFloor = 11;

const size_t numberOfFloors = maxFloor + 1;

const size_t maxTime =300;
int m = 40, n = 50, k = 300;
struct myParams
{
    Control* control = nullptr;
    bool started = false;
    size_t* time_exceed_btns_per_floors = new size_t[numberOfFloors];
    bool* is_elvs_fulled = new bool[numberOfElevators];


    void init() {
        for (int floor = 0; floor < numberOfFloors; floor++) {
            time_exceed_btns_per_floors[floor] = 0;
            //time_exeed_btns_elv[floor] = 0;
        }
        for (int elv = 0; elv < numberOfElevators; elv++) {
            is_elvs_fulled[elv] = 0;
        }
    };


    ~myParams() {
        delete[] time_exceed_btns_per_floors, is_elvs_fulled; //time_exeed_btns_elv;
    }


    //bool get_indicator(size_t floor, size_t elv) {
    //    return ((floor > control->getElevatorPosition(elv) && control->getElevatorIndicator(elv) == ElevatorIndicator::up) \
    //        || (floor < control->getElevatorPosition(elv) && control->getElevatorIndicator(elv) == ElevatorIndicator::down));
    //}


    bool get_indicator(size_t floor, size_t elv) {
        int c_up, c_dn = 0;
    }

    void time_exceed_increment(size_t elv) {
        for (size_t floor = 0; floor < numberOfFloors; floor++) {
            time_exceed_btns_per_floors[floor] += control->getElevatorButton(elv, floor) + control->getFloorDnButton(floor) \
                + control->getFloorUpButton(floor);
        }
    }


    double calc_priority_floor(size_t floor, size_t elv) {
        int is_btns_pressed = control->getFloorDnButton(floor) + control->getFloorUpButton(floor) \
            + control->getElevatorButton(elv, floor);
        if (is_btns_pressed == 0 || floor == control->getElevatorPosition(elv)) return -1;
        bool indicator = get_indicator(floor, elv), full = (is_elv_full_v1(elv, floor) || is_elvs_fulled[elv]);
        size_t distance = 1 / abs(control->getElevatorPosition(elv) - floor), time_exceed = time_exceed_btns_per_floors[floor];
        double a1 = 10000, a2 = m, a3 = n, a4 = k;
        return a1 * full + a2 * indicator + a3 * time_exceed + a4 * distance;
    }


    size_t get_next_dest(size_t elv, bool is_staying) {
        double max_priority = -100, dest_floor = 0, c_priority;
        size_t start = 0;
        size_t end = numberOfFloors;
        double pos = control->getElevatorPosition(elv);
        if (!is_staying) {
            if (control->getElevatorIndicator(elv) == ElevatorIndicator::up)
            {
                if ((int)pos == pos) start = pos;
                else start = pos + 1;
            }
            else 
            {
                if ((int)pos == pos) end = pos;
                else end = (int) pos + 1;
            }
        }
        for (size_t floor = start; floor < end; floor++) {
            c_priority = calc_priority_floor(floor, elv);

            std::cout << "\tfloor = " << floor << " priority = " << c_priority << "\n";

            
            if (c_priority > max_priority) {
                max_priority = c_priority;
                dest_floor = floor;
            }
            //std::cout << max_priority << "\n";
        }
        if (dest_floor > control->getElevatorPosition(elv)) control->SetElevatorIndicator(elv, ElevatorIndicator::up);
        else control->SetElevatorIndicator(elv, ElevatorIndicator::down);

        return dest_floor;
    }


    bool is_elv_full_v1(size_t elv, size_t floor) {
        static int passengers = 0;
        passengers -= control->getElevatorButton(elv, floor);
        passengers += (control->getFloorDnButton(floor) || control->getFloorUpButton(floor));

        if (passengers == elevatorCapacity) return true;
        return false;
    }


    size_t get_time_doors_closing(size_t elv) {
        static bool flag = false;
        size_t time = -1000;
        if (flag == false && control->isElevatorDoorsClosing(elv)) flag = true;
        if (flag) { 
            time = control->getCurrentTime(); 
            flag = false;
        }
        return time + control->timeClosing + 2;
    }
};




void CONTROLSYSTEM(Control& control, myParams& params);



int main(int argc, char** argv)
{
    Control control(numberOfFloors, numberOfElevators, elevatorCapacity);
    //control.ReadTimeTable("TimeTable/timetable125.csv");
   
    control.AddPassengerToQueue({ 10, 10, 3, 600, 0,0,0 });
    control.AddPassengerToQueue({ 15, 7, 11, 600, 0,0,0 });
    //control.AddPassengerToQueue({ 5, 5, 3, 300, 0.01, 0.20, 0.50 });
    myParams params;
    do
    {
        control.MakeStep();
        CONTROLSYSTEM(control, params);


        control.PrintElevatorState(0, "fileElev0.txt"); //Вывод состояния лифта #0 в файл
        control.PrintElevatorState(1, "fileElev1.txt"); //Вывод состояния лифта #1 в файл

        control.PrintButtonsState("fileButtons.txt");   //Вывод состояния кнопок в файл

        control.PrintPassengerState("filePassengers.txt");  //Вывод статистики пассажиров в файл

    } while (control.getCurrentTime() <= maxTime);

    control.PrintStatistics(true, "Statistics.txt");

    return 0;
}
//int main() {
//    for (int m0 = 1; m0 < 300; m0 += 50) {
//        for (int n0 = 1; n0 < 300; n0 += 50) {
//            for (int k0 = 1; k0 < 500; k0 += 50) {
//                m = m0;
//                n = n0;
//                k = k0;
//                main_(0, 0);
//            }
//        }
//    }
//    return 0;
//
//}

void CONTROLSYSTEM(Control& control, myParams& params)
{
    if (control.getCurrentTime() == 1)
    {
        params.init();
        params.control = &control;
        control.SetElevatorDestination(1, maxFloor);
        control.SetElevatorIndicator(1, ElevatorIndicator::up); // поменял на both(up)
    }

    if (!params.started)
    {
        size_t nUp = std::count(control.getFloorUpButtons().begin(), control.getFloorUpButtons().end(), true);
        size_t nDn = std::count(control.getFloorDnButtons().begin(), control.getFloorDnButtons().end(), true);

        if (nUp + nDn > 0)
        {
            params.started = true;
        }
    }

    for (size_t elv = 0; elv < 1; ++elv)
    {
        params.time_exceed_increment(elv);

        size_t time_to_check = params.get_time_doors_closing(elv);
        if (time_to_check > 0) {
            size_t last_floor = round(control.getElevatorPosition(elv));
            if (control.getCurrentTime() == time_to_check \
                && (control.getFloorDnButton(last_floor) + control.getFloorUpButton(last_floor)) > 0) params.is_elvs_fulled[elv] = true;
        }
        size_t global_dest = params.get_next_dest(elv, control.isElevatorStaying(elv));

        if ((params.started) && (control.isElevatorAchievedDestination(elv)))
        {
            control.SetElevatorIndicator(elv, ElevatorIndicator::both);
            size_t pos = control.getElevatorPosition(elv);
            if (((control.getFloorDnButton(pos) || control.getFloorUpButton(pos)) - control.getElevatorButton(elv, pos)) \
                < 0) params.is_elvs_fulled[elv] = false;

            control.unsetDnButton(pos);
            control.unsetUpButton(pos);
            params.time_exceed_btns_per_floors[pos] = 0;

        }
        control.SetElevatorDestination(elv, global_dest);
        std::cout << control.getCurrentTime() << " " << control.getElevatorDestination(elv) << "\n";


    }
}
