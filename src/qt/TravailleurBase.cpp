#include "TravailleurBase.h"

#include <algorithm>

#include "ImageQt.h"

TravailleurBase::TravailleurBase(QObject* parent) : QObject(parent) {}

void TravailleurBase::configurer(const nis::OptionsBase& options, const QStringList& fichiers,
                                 const QString& dossierSortie) {
  options_ = options;
  fichiers_ = fichiers;
  dossierSortie_ = dossierSortie;
  resultat_.reset();
  arret_.store(false);
}

void TravailleurBase::demanderArret() { arret_.store(true); }

void TravailleurBase::executer() {
  std::vector<std::string> fichiers;
  fichiers.reserve(static_cast<std::size_t>(fichiers_.size()));
  for (const QString& f : fichiers_) fichiers.push_back(f.toStdString());
  nis::ConstructeurBase b(options_, pontqt::chargeurFichier());
  auto resultat = std::make_shared<nis::ResultatBase>();
  *resultat = b.construire(
      fichiers, dossierSortie_.toStdString(),
      [this](double fraction, const std::string& libelle) {
        if (arret_.load()) return false;
        const int pct =
            std::max(0, std::min(100, static_cast<int>(fraction * 100.0)));
        emit progression(pct, QString::fromStdString(libelle));
        return !arret_.load();
      });
  if (arret_.load()) resultat->annule = true;
  resultat_ = std::move(resultat);
  emit termine(resultat_->annule);
}
