#include "facedatabase.h"
#include <sqlite3.h>
#include <cstring>
#include <cmath>

// ---------------- COSINE SIMILARITY ----------------
static double cosineSimilarity(const std::vector<float> &a,
                               const std::vector<float> &b)
{
    double dot = 0.0;
    double na = 0.0;
    double nb = 0.0;

    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }

    if (na == 0.0 || nb == 0.0)
        return 0.0;

    return dot / (std::sqrt(na) * std::sqrt(nb));
}

// ---------------- LOAD DATABASE ----------------
bool FaceDatabase::load(const std::string &dbPath)
{
    sqlite3 *db = nullptr;

    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK)
        return false;

    const char *sql =
        "SELECT colaborador_id, encoding "
        "FROM rostos";

    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }

    faces.clear();

    while (sqlite3_step(stmt) == SQLITE_ROW) {

        int colaboradorId = sqlite3_column_int(stmt, 0);

        const void *blob = sqlite3_column_blob(stmt, 1);
        int blobSize = sqlite3_column_bytes(stmt, 1);

        // Segurança: MobileFaceNet = 128 floats = 512 bytes
        if (blobSize != 512)
            continue;

        std::vector<float> embedding(128);
        std::memcpy(embedding.data(), blob, 512);

        faces.push_back({ colaboradorId, embedding });
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return true;
}

// ---------------- FIND MATCH ----------------
int FaceDatabase::findBestMatch(const std::vector<float> &embedding,
                                double &bestScore) const
{
    int bestId = -1;
    bestScore = 0.0;

    for (const auto &f : faces) {
        double score = cosineSimilarity(embedding, f.embedding);

        if (score > bestScore) {
            bestScore = score;
            bestId = f.colaboradorId;
        }
    }

    return bestId;
}
