// Classification d'une base executee hors du fil d'interface.
//
// Remplace `Ccommencer::Oncommencer` (boucle Sleep + barre simulee) par un
// vrai travail de fond : ConstructeurBase tourne ici, la progression reelle
// et la fin sont renvoyees au fil d'interface par signaux.
#ifndef NIS_QT_TRAVAILLEUR_BASE_H
#define NIS_QT_TRAVAILLEUR_BASE_H

#include <QObject>
#include <QStringList>
#include <atomic>
#include <memory>

#include "core/ConstructeurBase.h"

class TravailleurBase : public QObject {
  Q_OBJECT
 public:
  explicit TravailleurBase(QObject* parent = nullptr);

  void configurer(const nis::OptionsBase& options, const QStringList& fichiers,
                  const QString& dossierSortie);
  void demanderArret();

  // Resultat disponible apres le signal termine (succes ou annulation).
  std::shared_ptr<nis::ResultatBase> resultat() const { return resultat_; }

 public slots:
  void executer();

 signals:
  void progression(int pourcentage, const QString& libelle);
  void termine(bool annule);

 private:
  nis::OptionsBase options_;
  QStringList fichiers_;
  QString dossierSortie_;
  std::atomic<bool> arret_{false};
  std::shared_ptr<nis::ResultatBase> resultat_;
};

#endif  // NIS_QT_TRAVAILLEUR_BASE_H
