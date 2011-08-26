#ifndef ANALIZER_H
#define ANALIZER_H

#include "interfaces/error.h"
#include "interfaces/lineprop.h"
#include "dataformats/ast.h"
#include "dataformats/ast_module.h"
#include "dataformats/ast_algorhitm.h"
#include "interfaces/analizerinterface.h"

#include <QtCore>

namespace KumirAnalizer {

class Analizer : public QObject
{
    Q_OBJECT
public:

    explicit Analizer(class KumirAnalizerPlugin * plugin);

    ~Analizer();

    /**
      * Set application-wide (while initialization)
      * Kumir source language (Russian, Ukrainian, etc.)
      */
    static void setSourceLanguage(const QLocale::Language & language);

public slots:

    void changeSourceText(const QList<Shared::ChangeTextTransaction> & changes);
    void setHiddenText(const QString & text, int baseLineNo);
    void setHiddenBaseLine(int lineNo);

    QStringList algorhitmsAvailableFor(int lineNo) const;
    QStringList globalsAvailableFor(int lineNo) const;
    QStringList localsAvailableFor(int lineNo) const;

    QList<Shared::Error> errors() const;

    QList<Shared::LineProp> lineProperties() const;

    Shared::LineProp lineProp(const QString & text) const;

    QStringList algorhitmNames() const;
    QStringList moduleNames() const;


    QList<QPoint> lineRanks() const;

    QStringList imports() const;

    const AST::Data * abstractSyntaxTree() const;

private:
    const AST::Module * findModuleByLine(int lineNo) const;
    const AST::Algorhitm * findAlgorhitmByLine(const AST::Module * mod, int lineNo) const;
    class AnalizerPrivate * d;

};

} // namespace KumirAnalizer

#endif // ANALIZER_H
