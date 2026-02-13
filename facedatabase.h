#ifndef FACEDATABASE_H
#define FACEDATABASE_H

#include <vector>
#include <string>

struct FaceEntry
{
    int colaboradorId;
    std::vector<float> embedding; // 128 floats
};

class FaceDatabase
{
public:
    FaceDatabase() = default;

    // Carrega todos os rostos para memória
    bool load(const std::string &dbPath);

    // Compara embedding atual com a BD
    int findBestMatch(const std::vector<float> &embedding,
                      double &bestScore) const;

private:
    std::vector<FaceEntry> faces;
};

#endif
