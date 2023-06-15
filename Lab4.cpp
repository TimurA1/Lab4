#include <iostream>
#include <ctime>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <string>
#include <map>
#include <deque>
#include <random>

#define uint unsigned long long

using namespace std;

string inputFile = "data.txt";
string outputFile = "output.txt";
string timeStampsFile = "timestamps.txt";

ofstream fout(outputFile);

const uint sampleSize = 101;
const uint sampleNumber = 10;
const uint maxBound = 1 << 20;

const int batchNum = 6;
const int dims[batchNum] = { 1000, 5000, 10000, 50000, 100000, 1000000 };

class RandomEngine1 {
    uint seed = 0;
    uint multiply = 48271;
    uint plus = 0;
    uint modulus = 1 << 31 - 1;

public:
    void setSeed(uint seed) {
        this->seed = seed;
    }

    uint getLimitlessNumber() {
        seed = (seed * multiply + plus) % modulus;
        return seed;
    }

    uint getNumber() {
        seed = (seed * multiply + plus) % modulus;
        return seed / (modulus / maxBound);
    }

    vector<uint> getVector(size_t size) {
        vector<uint> v;
        v.resize(size);

        for (size_t i = 0; i < size; ++i)
            v[i] = this->getNumber();

        return v;
    }

    void shuffleVector(vector<uint>& v) {
        random_shuffle(v.begin(), v.end());
    }

    uint getRandomElement(const vector<uint>& v) {
        if (!v.empty()) {
            size_t index = getNumber() % v.size();
            return v[index];
        }
        return 0; 
};

class RandomEngine2 {
        unsigned int a = 97, b = 33;
        std::deque<uint> d;
        unsigned long long modulus = (1ull << 32) - 1;
        RandomEngine1 eng;

    public:
        void setSeed(uint seed) {
            eng.setSeed(seed);
            d.clear();
            for (unsigned int i = 0; i < a; ++i) {
                d.push_front(eng.getLimitlessNumber() % modulus);
            }
        }

        uint getNumber() {
            uint newNumber;
            if (d[a - 1] > d[b - 1])
                newNumber = (d[a - 1] - d[b - 1]) % modulus;
            else
                newNumber = (d[b - 1] - d[a - 1] + 1) % modulus;

            d.push_front(newNumber);
            d.pop_back();
            return newNumber % maxBound;
        }

        std::vector<uint> getVector(size_t size) {
            std::vector<uint> v(size);

            for (size_t i = 0; i < size; ++i)
                v[i] = this->getNumber();

            return v;
        }

        void shuffleVector(std::vector<uint>& v) {
            std::random_shuffle(v.begin(), v.end());
        }

        uint getRandomElement(const std::vector<uint>& v) {
            if (!v.empty()) {
                size_t index = getNumber() % v.size();
                return v[index];
            }
            return 0; // or throw an exception if the vector is empty
        }
    };

double mean(vector<uint>& v) {
    unsigned long long sum = 0;

    for (auto& n : v)
        sum += n;

    return sum / static_cast<double>(v.size());
}

double deviation(double mean, vector<uint>& v) {
    double sum = 0;

    for (auto& n : v)
        sum += (n - mean) * (n - mean);

    return sqrt(sum / v.size());
}

double variationCoeff(double deviation, double mean) {
    return deviation / mean;
}

double chiSquare(vector<uint>& vec) {
    //считается, что в v лежат целые числа, до maxBound, поэтому их надо нормировать
    vector<double> v;
    for (int i = 0; i < vec.size(); ++i)
        v.push_back(vec[i] / static_cast<double>(maxBound - 1));

    const unsigned int n = v.size();
    const unsigned int k = 1 + 3.322 * log(n);
    const double p = 1 / static_cast<double>(k);

    vector<unsigned int> n_i(k, 0);

    for (auto& u : v)
        for (int j = 0; j < k; ++j)
            if (p * j <= u && p * (j + 1) > u)
                n_i[j] += 1;

    double chi = 0;

    //суммирование
    for (auto& u : n_i)
        chi += ((static_cast<double>(u) - p * n) * (static_cast<double>(u) - p * n)) / (p * n);

    return chi;
}

void writeTime(string title, std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end, int divideBy = 1) {
    fout << title;

    fout << chrono::duration_cast<chrono::microseconds>(end - start).count() / divideBy << " [микросекунд]\n";
}

void writeSampleInfo(vector<uint>& vec) {
    double m = mean(vec);
    double d = deviation(m, vec);
    double v = variationCoeff(d, m);
    double chi = chiSquare(vec);

    fout << "Среднее: " << m << "\nОтклонение: " << d << "\nКоэффициент вариации: " << v
        << "\nКритерий хи-квадрат: " << chi << "\n\n";
}

int main()
{
    setlocale(LC_ALL, "Russian");

    std::chrono::steady_clock::time_point start, end;

    RandomEngine1 eng;

    fout << "Первый генератор:\n" << '\n';

    eng.setSeed(time(NULL));

    for (int i = 0; i < sampleNumber; ++i) {
        vector<uint> v = eng.getVector(sampleSize);
        writeSampleInfo(v);
    }

    start = std::chrono::steady_clock::now();

    for (int i = 0; i < batchNum; ++i) {
        //генерация с засечением времени
        for (int j = 0; j < dims[i]; ++j)
            const uint num = eng.getNumber();

        end = std::chrono::steady_clock::now();
        writeTime("На генерацию " + to_string(dims[i]) + " значений ушло : ", start, end);
        start = end;
    }

    RandomEngine2 eng1;

    eng1.setSeed(time(NULL));

    fout << "\nВторой генератор:\n" << '\n';

    for (int i = 0; i < sampleNumber; ++i) {
        vector<uint> v = eng1.getVector(sampleSize);
        writeSampleInfo(v);
    }

    start = std::chrono::steady_clock::now();

    for (int i = 0; i < batchNum; ++i) {
        //генерация с засечением времени
        for (int j = 0; j < dims[i]; ++j)
            const uint num = eng1.getNumber();

        end = std::chrono::steady_clock::now();
        writeTime("На генерацию " + to_string(dims[i]) + " значений ушло : ", start, end);
        start = end;
    }

    fout << "\nСравнение с mt19937:\n" << '\n';

    mt19937 mt_rand(time(0));
    for (int l = 0; l < sampleNumber; ++l) {
        vector<uint> vRand(sampleSize, 0);
        for (int i = 0; i < sampleSize; ++i)
            vRand[i] = mt_rand() % maxBound;
        writeSampleInfo(vRand);
    }

    start = std::chrono::steady_clock::now();

    for (int i = 0; i < batchNum; ++i) {
        //генерация с засечением времени
        for (int j = 0; j < dims[i]; ++j)
            const uint num = mt_rand();

        end = std::chrono::steady_clock::now();
        writeTime("На генерацию " + to_string(dims[i]) + " значений ушло : ", start, end);
        start = end;
    }


    return 0;
}
