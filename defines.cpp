#include "defines.h"
#include <chrono>
#include <iostream>

void Stringsplit(const std::string &str, const char split, std::vector<std::string> &res) {
    res.clear();
    if (str == "") return;
    std::string strs = str + split;
    size_t pos = strs.find(split);

    while (pos != strs.npos) {
        std::string temp = strs.substr(0, pos);
        res.push_back(temp);
        strs = strs.substr(pos + 1, strs.size());
        pos = strs.find(split);
    }
}

std::string GetTimeStr() {
    auto t = time(nullptr);
    auto* now = localtime(&t);
    char time[32] = {0};
    //sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
    sprintf(time, "%02d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);
    return std::string(time);
}

int GetNowTick() {
    auto t = time(nullptr);
    return (int)t;
}

int GetClockTick() {
    auto t = time(nullptr);
    auto* now = localtime(&t);
    char time[32] = {0};
    sprintf(time, "%02d%02d%02d000", now->tm_hour, now->tm_min, now->tm_sec);
    return atoi(time);
}

void trim(std::string &s) {
    int index = 0;
    if(!s.empty()) {
        while( (index = s.find(' ',index)) != std::string::npos) {
            s.erase(index,1);
        }
    }
}

std::string GetThreadID() {
    auto tid = std::this_thread::get_id();
    std::stringstream ss;
    ss << tid;
    return ss.str();
}

long long int GetUs() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    return microseconds.count();
}

int FindOrderNo(std::vector<TORALEV2API::TTORATstpLongSequenceType>& vec, TORALEV2API::TTORATstpLongSequenceType OrderNo) {
    int left = 0;
    int right = (int)vec.size() - 1;	// 定义了target在左闭右闭的区间内，[left, right]
    while (left <= right) {	//当left == right时，区间[left, right]仍然有效
        int middle = left + ((right - left) / 2);//等同于 (left + right) / 2，防止溢出
        if (vec.at(middle) > OrderNo) {
            right = middle - 1;	//target在左区间，所以[left, middle - 1]
        } else if (vec.at(middle) < OrderNo) {
            left = middle + 1;	//target在右区间，所以[middle + 1, right]
        } else {	//既不在左边，也不在右边，那就是找到答案了
            return middle;
        }
    }
    return -1;
}

int GetTotalIndex(TORASTOCKAPI::TTORATstpPriceType UpperLimitPrice, TORASTOCKAPI::TTORATstpPriceType LowerLimitPrice) {
    auto diffPrice = UpperLimitPrice - LowerLimitPrice;
    auto tempIndex = diffPrice / 0.01;
    auto totalIndex = int(tempIndex) + 1;
    if (int(tempIndex * 10) % 10 >= 5) {
        totalIndex += 1;
    }
    return totalIndex;
}
