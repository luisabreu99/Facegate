#ifndef DATABASE_H
#define DATABASE_H

#include <QString>

class Database {
public:
    static bool inicializar();

    static int adicionarColaborador(const QString &nome,
                                    const QString &apelido,
                                    const QString &dataNascimento,
                                    const QString &pasta);

    // Mantemos a versão antiga (2 args) para não partir nada
    static void adicionarFoto(int colaboradorId, const QString &caminhoFoto);
    static QVector<QPair<int, QByteArray>> obterEncodings();
    // Nova: já aceita o encoding (usaremos no próximo passo)
    static void adicionarFoto(int colaboradorId,
                              const QString &caminhoFoto,
                              const QByteArray &encoding);
    static QString obterUltimaMarcacao(int colaboradorId);
    static bool registarMarcacao(int colaboradorId, const QString &tipo);
};

#endif // DATABASE_H
