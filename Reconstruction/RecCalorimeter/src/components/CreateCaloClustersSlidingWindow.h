#ifndef RECCALORIMETER_CREATECALOCLUSTERSSLIDINGWINDOW_H
#define RECCALORIMETER_CREATECALOCLUSTERSSLIDINGWINDOW_H

// GAUDI
#include "GaudiAlg/GaudiAlgorithm.h"

// FCCSW
#include "FWCore/DataHandle.h"
#include "DetSegmentation/GridPhiEta.h"
class IGeoSvc;

// datamodel
#include "datamodel/CaloHit.h"
namespace fcc {
class CaloHitCollection;
class CaloClusterCollection;
}

// DD4hep
#include "DD4hep/Readout.h"
namespace DD4hep {
namespace DDSegmentation {
  class Segmentation;
}
}

// Cluster
struct cluster {
  float transEnergy;
  float eta;
  float phi;
};

/** @class CreateCaloClustersSlidingWindow
 *
 *  Algorithm for creating calorimeter clusters from cells.
 *
 *  Reconstruction is performed for the calorimeter with readout '\b readoutName' and volume ID retrieved using '\b fieldNames' and '\b fieldValues'.
 *
 *  Sliding window algorithm:
 *  1. Create calorimeter towers.
 *     A tower contains all cells within certain eta and phi (tower size: '\b deltaEtaTower', '\b deltaPhiTower').
 *     Distance in r plays no role in this algorithm.
 *     TODO: Currently there is no support of cell splitting, so each cell should be completely inside the tower
 *     and that can be achieved using `GridEtaPhi` segmentation.
 *  2. Find local maxima.
 *     Local maxima are found using the sliding window of a fixed size in phi x eta ('\b nEtaWindow' '\b nPhiWindow' in units of tower size).
 *     If a local max is found and its energy is above threshold ('\b energyThreshold'), it is added to the preclusters list.
 *     Each precluster contains the barycentre position and the transverse energy.
 *     Position is recalculated using the window size in eta x phi ('\b nEtaPosition', '\b nPhiPosition')
 *     that may be smaller than the sliding window to reduce the noise influence. Both windows are centred at the same tower.
 *     The energy of the precluster is the energy calculated using the sliding window.
 *  3. Remove duplicates.
 *     If two pre-clusters are found next to each other (within window '\b nEtaDuplicates', '\b nPhiDuplicates'), the pre-cluster with lower energy is removed.
 *     TODO: Currently there is no support on energy sharing between clusters, so if duplicate window is smaller than sliding window, some towers may be taken twice (nstead of the weighted energy).
 *  4. Build clusters.
 *     Clusters are created using the pre-clusters energy (energy of towers within the sliding window).
 *     Position is calculated from the barycentre position and the inner radius of the detector.
 *     For each cluster the cell collection is searched and all those inside the cluster are attached.
 *     TODO: Final cluster may have a different size, so recalculation of energy around the tower containing barycentre should be done.
 *  Sliding window performs well for electrons/gamma reconstruction.
 *  Topological clusters should be better for jets.
 *
 *  @author Jana Faltova
 *  @author Anna Zaborowska
 */

class CreateCaloClustersSlidingWindow : public GaudiAlgorithm {
public:
  CreateCaloClustersSlidingWindow(const std::string& name, ISvcLocator* svcLoc);
  /**  Initialize.
   *   @return status code
   */
  StatusCode initialize();
  /**  Execute.
   *   Perform the sliding window algorithm and build clusters.
   *   @return status code
   */
  StatusCode execute();
  /**  Finalize.
   *   @return status code
   */
  StatusCode finalize();

private:
  /**  Prepare calorimeter towers.
   *  Create empty towers for the calorimeter.
   */
  void prepareTowers();
  /**  Build calorimeter towers.
   *  Tower is segmented in eta and phi, with the energy from all layers (no r segmentation).
   *  Cuurently the cells need to be included only in one tower.
   *  TODO: split cell energy into more towers if cell size is larger than the tower
   */
  void buildTowers();
  /**  Get the tower IDs in eta.
   *   @param[in] aEta Position of the calorimeter cell in eta
   *   @return ID (eta) of a tower
   */
  int idEta(float aEta) const;
  /**  Get the tower IDs in phi.
   *   @param[in] aPhi Position of the calorimeter cell in phi
   *   @return ID (phi) of a tower
   */
  int idPhi(float aPhi) const;
  /**  Get the eta position of the centre of the tower.
   *   @param[in] aIdEta ID (eta) of a tower
   *   @return Position of the centre of the tower
   */
  float eta(int aIdEta) const;
  /**  Get the phi position of the centre of the tower.
   *   @param[in] aIdPhi ID (phi) of a tower
   *   @return Position of the centre of the tower
   */
  float phi(int aIdPhi) const;
  /**  Correct way to access the neighbour of the phi tower, taking into account the full coverage in phi.
   *   Full coverage means that first tower in phi, with ID = 0 is a direct neighbour
   *   of the last tower in phi with ID = m_nPhiTower - 1).
   *   @param[in] aIPhi requested ID of a phi tower, may be < 0 or >= m_nPhiTower
   *   @return  ID of a tower - shifted and corrected (in [0, m_nPhiTower) range)
   */
  uint phiNeighbour(int aIPhi) const;
  /// Pointer to the geometry service
  SmartIF<IGeoSvc> m_geoSvc;
  /// Handle for calo cells (input collection)
  DataHandle<fcc::CaloHitCollection> m_cells;
  /// Handle for calo clusters (output collection)
  DataHandle<fcc::CaloClusterCollection> m_clusters;
  /// PhiEta segmentation (owned by DD4hep)
  DD4hep::DDSegmentation::GridPhiEta* m_segmentation;
  // calorimeter towers
  std::vector<std::vector<float>> m_towers;
  /// Vector of pre-clusters
  std::vector<cluster> m_preClusters;
  /// Name of the detector readout
  Gaudi::Property<std::string> m_readoutName{"readoutName", "ECalHitsPhiEta"};
  /// Name of the fields describing the segmented calorimeter
  Gaudi::Property<std::vector<std::string>> m_fieldNames{"fieldNames", {}};
  /// Values of the fields describing the segmented calorimeter
  Gaudi::Property<std::vector<int>> m_fieldValues{"fieldValues", {}};
  /// Volume ID of the volume with cells to calculate
  uint64_t m_volumeId;
  /// Size of the tower in eta
  Gaudi::Property<float> m_deltaEtaTower{"deltaEtaTower", 0.01, "Size of the tower in eta"};
  /// Size of the tower in phi
  Gaudi::Property<float> m_deltaPhiTower{"deltaPhiTower", 0.01, "Size of the tower in phi"};
  /// number of towers in eta (calculated from m_deltaEtaTower and the eta size of the first layer)
  int m_nEtaTower;
  /// Number of towers in phi (calculated from m_deltaPhiTower)
  int m_nPhiTower;
  /// Size of the window in eta for pre-clusters (in units of tower size)
  Gaudi::Property<int> m_nEtaWindow{"nEtaWindow", 5};
  /// Size of the window in phi for pre-clusters (in units of tower size)
  Gaudi::Property<int> m_nPhiWindow{"nPhiWindow", 5};
  /// Size of the window in eta for cluster position calculation (in units of tower size)
  Gaudi::Property<int> m_nEtaPosition{"nEtaPosition", 3};
  /// Size of the window in phi for cluster position calculation (in units of tower size)
  Gaudi::Property<int> m_nPhiPosition{"nPhiPosition", 3};
 /// Size of the window in eta for the overlap removal (in units of tower size)
  Gaudi::Property<int> m_nEtaDuplicates{"nEtaDuplicates", 2};
  /// Size of the window in phi for the overlap removal (in units of tower size)
  Gaudi::Property<int> m_nPhiDuplicates{"nPhiDuplicates", 2};
  /// Energy threshold for cluster finding
  Gaudi::Property<float> m_energyThreshold{"energyThreshold", 3};
  /// Energy threshold in the window for the position calculation
  Gaudi::Property<float> m_energyPosThreshold{"energyPosThreshold", 0.00001};
  /// Flag if a check on local maxima in phi should be done (temporary, to test the algorithm)
  Gaudi::Property<bool> m_checkPhiLocalMax{"checkPhiLocalMax", true};
  /// Flag if a check on local maxima in eta should be done (temporary, to test the algorithm)
  Gaudi::Property<bool> m_checkEtaLocalMax{"checkEtaLocalMax", true};
};

#endif /* RECCALORIMETER_CREATECALOCLUSTERSSLIDINGWINDOW_H */
