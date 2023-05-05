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

const size_t maxTime = 6000;
int m, n, k;
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


    bool get_indicator(size_t floor, size_t elv) {
        return ((floor > control->getElevatorPosition(elv) && control->getElevatorIndicator(elv) == ElevatorIndicator::up) \
            || (floor < control->getElevatorPosition(elv) && control->getElevatorIndicator(elv) == ElevatorIndicator::down));
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
        if (is_btns_pressed == 0 || floor == control->getElevatorPosition(elv) == floor) return -1;
        bool indicator = get_indicator(floor, elv), full = (is_elv_full_v1(elv, floor) || is_elvs_fulled[elv]);
        size_t distance = 1 / abs(control->getElevatorPosition(elv) - floor), time_exceed = time_exceed_btns_per_floors[floor];
        double a1 = 10000, a2 = 40, a3 = 40, a4 = 300;
        return a1 * full + a2 * indicator + a3 * time_exceed + a4 * distance;
    }


    size_t get_next_dest(size_t elv) {
        size_t max_priority = 0, dest_floor = 0, c_priority;
        for (size_t floor = 0; floor < numberOfFloors; floor++) {
            c_priority = calc_priority_floor(floor, elv);
            if (c_priority > max_priority) {
                max_priority = c_priority;
                dest_floor = floor;
            }
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
    control.ReadTimeTable("TimeTable/timetable125.csv");
   

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

    for (size_t elv = 0; elv < numberOfElevators; ++elv)
    {
        params.time_exceed_increment(elv);

        size_t time_to_check = params.get_time_doors_closing(elv);
        if (time_to_check > 0) {
            size_t last_floor = round(control.getElevatorPosition(elv));
            if (control.getCurrentTime() == time_to_check \
                && (control.getFloorDnButton(last_floor) + control.getFloorUpButton(last_floor)) > 0) params.is_elvs_fulled[elv] = true;
        }

        if ((params.started) && (control.isElevatorAchievedDestination(elv)))
        {

            size_t pos = control.getElevatorPosition(elv);
            if (((control.getFloorDnButton(pos) || control.getFloorUpButton(pos)) - control.getElevatorButton(elv, pos)) \
                < 0) params.is_elvs_fulled[elv] = false;
            control.unsetDnButton(pos);
            control.unsetUpButton(pos);
            size_t global_dest = params.get_next_dest(elv);
            params.time_exceed_btns_per_floors[pos] = 0;

            control.SetElevatorDestination(elv, global_dest);
        }

    }
}
