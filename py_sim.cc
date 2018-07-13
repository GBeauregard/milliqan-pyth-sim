#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include "Pythia8/Pythia.h"

#include "TFile.h"
#include "TROOT.h"
#include "TTree.h"

using namespace Pythia8;

// struct of information from each event that will be filled in ROOT tree
typedef struct {
  Int_t mother_id;    // particle ID of mother
  Double_t pT;        // pT of mCP ("muon")
  Double_t eta;       // eta (pseudorapidity) of mCP ("muon")
  Double_t ev_pTHat;  // event's pTHat
  Bool_t charge;      // true, +1; false, -1
  UInt_t event_num;   // event number muon came from
} mCP_event;

// Takes vector of {dbl,dbl} pts and returns unique polynomial approx at
// x=evalpt. Uses Lagrange polynomial method.
double poly_approx(vector<vector<double>> pts, double evalpt) {
  double output = 0.0;
  for (int i = 0; i < pts.size(); i++) {
    double l = 1.0;
    for (int j = 0; j < pts.size(); j++)
      if (j != i) l *= (evalpt - pts[j][0]) / (pts[i][0] - pts[j][0]);
    output += pts[i][1] * l;
  }
  return output;
}

int main(int argc, char **argv) {
  // set default options
  // number of events to generate
  int nEvent = 1000;
  // mCP mass in GeV
  double mCPmass = 0.05;
  // jet pT (pTHat) cut in GeV
  double pTcut = 50.;
  // name of output root file with events
  TString output_file = "out.root";

  // process commandline arguments
  int c;
  while ((c = getopt(argc, argv, "f:n:m:p:")) != -1) switch (c) {
      case 'f':  // output root file name
        output_file = optarg;
        break;
      case 'n':  // number of events to generate
        nEvent = std::stoi(optarg);
        break;
      case 'm':  // mCP mass in GeV
        mCPmass = std::stod(optarg);
        break;
      case 'p':  // jet pT (pTHat) cut in GeV
        pTcut = std::stod(optarg);
        break;
      case '?':
        cout << "Error: Invalid option" << endl;
        return EXIT_FAILURE;
    }

  // Generator
  Pythia pythia;
  // Turn on all hard QCD processes
  pythia.readString("HardQCD:all = on");

  // Initialization, pp beam @ 13 TeV
  pythia.readString("Beams:idA = 2212");
  pythia.readString("Beams:idB = 2212");
  pythia.readString("Beams:eCM = 13000");

  // Apply pT cut
  std::ostringstream strspT;
  strspT << pTcut;
  pythia.readString("PhaseSpace:pTHatMin = " + strspT.str());

  // eanble this to see output of all particle data
  // pythia.particleData.listAll();

  // Vector of hadrons and channels to adjust the branching ratios with a line.
  // Each item contains: {hadron_id, decay channel particle id, decay_channel,
  // optional IDs of non-mCP products}
  vector<vector<int>> lchs;

  // Vector of hadrons and channels to adjust the branching ratios with a
  // quadratic. Each item contains: {hadron_id, e_channel, mu_channel, optional
  // IDs of non-mCP products}
  vector<vector<int>> qchs;

  // 113  rho0                                3   0   0    0.77549
  //          4     1   0.0000471    0       11      -11
  //          5     1   0.0000454    0       13      -13
  qchs.push_back({113, 4, 5});

  // 221  eta                                 1   0   0    0.54785
  //          5     1   0.0069003   11       22       11      -11
  //          6     1   0.0003100   11       22       13      -13
  //          7     1   0.0000060    0       13      -13
  qchs.push_back({221, 5, 6});
  lchs.push_back({221, 13, 7});

  // 223  omega                               3   0   0    0.78265
  //          4     1   0.0007765   11      111       11      -11
  //          5     1   0.0001311   11      111       13      -13
  //          6     1   0.0000728    0       11      -11
  //          7     1   0.0000900    0       13      -13
  qchs.push_back({223, 4, 5, 111});
  qchs.push_back({223, 6, 7});

  //  331  eta'                                1   0   0    0.95778
  //          6     1   0.0001076    0       13      -13       22
  lchs.push_back({331, 13, 6});

  // 333  phi                                 3   0   0    1.01946
  //          8     1   0.0002956    0       11      -11
  //          9     1   0.0002872    0       13      -13
  qchs.push_back({333, 8, 9});

  // 443  J/psi                               3   0   0    3.09692
  //          1     1   0.0594000    0       11      -11
  //          2     1   0.0593000    0       13      -13
  qchs.push_back({443, 1, 2});

  // 511  B0               Bbar0              1   0   0    5.27958
  //        264     1   0.0000006   11      311       11      -11
  //        265     1   0.0000006   11      311       13      -13
  //        282     1   0.0000018   11      313       11      -11
  //        283     1   0.0000014   11      313       13      -13
  //        866     2   0.0000050   12        1       -3       11      -11
  //        867     2   0.0000025   12        1       -3       13      -13
  //        887     3   0.0000050   12       -3        1       11      -11
  //        888     3   0.0000025   12       -3        1       13      -13
  qchs.push_back({511, 264, 265, 311});
  qchs.push_back({511, 282, 283, 313});
  qchs.push_back({511, 866, 867, 1, -3});
  qchs.push_back({511, 887, 888, -3, 1});

  // 521  B+               B-                 1   3   0    5.27925
  //        224     1   0.0000006   11      321       11      -11
  //        225     1   0.0000006   11      321       13      -13
  //        251     1   0.0000018   11      323       11      -11
  //        252     1   0.0000014   11      323       13      -13
  //        722     2   0.0000050   12        2       -3       11      -11
  //        723     2   0.0000025   12        2       -3       13      -13
  //        732     3   0.0000050   12       -3        2       11      -11
  //        733     3   0.0000025   12       -3        2       13      -13
  qchs.push_back({521, 224, 225, 321});
  qchs.push_back({521, 251, 252, 323});
  qchs.push_back({521, 722, 723, 2, -3});
  qchs.push_back({521, 732, 733, -3, 2});

  // 531  B_s0             B_sbar0            1   0   0    5.36677
  //        109     1   0.0000023   11      333       11      -11
  //        110     1   0.0000023   11      333       13      -13
  qchs.push_back({531, 109, 110, 333});

  // 553  Upsilon                             3   0   0    9.46030
  //          2     1   0.0238000    0       11      -11
  //          3     1   0.0248000    0       13      -13
  qchs.push_back({553, 2, 3});

  // 100443  psi(2S)                          3   0   0    3.68
  //          0     1   0.0073500    0       11      -11
  //          1     1   0.0073000    0       13      -13
  qchs.push_back({100443, 0, 1});

  // 100553  Upsilon(2S)                      3   0   0   10.02
  //          0     1   0.0191000    0       11      -11
  //          1     1   0.0193000    0       13      -13
  qchs.push_back({100553, 0, 1});

  // 200553  Upsilon(3S)                      3   0   0   10.35
  //          0     1   0.0000000    0       11      -11
  //          1     1   0.0379639    0       13      -13
  qchs.push_back({200553, 0, 1});

  // initialize points for linear branching ratio approximation
  vector<vector<double>> line_pts{
      {0., 0.},   // {0.5*(decaying hadron mass - non-muon product masses), 0}
      {0., 0.}};  // {m_decaying_to, branch_ratio decaying to}

  // loop through decay channels and set new branching ratios for linear
  // approximation
  for (vector<int> ch : lchs) {
    int had_id = ch[0];
    int dec_id = ch[1];
    int dec_ch = ch[2];
    double sub_mass = 0.0;
    // loop through extra particles to subtract mass
    for (int i = 0; i < ch.size(); i++) {
      if (i >= 3) sub_mass += pythia.particleData.m0(ch[i]);
    }
    // set 0 mass then id mass and branching ratio according to IDs
    line_pts[0][0] = 0.5 * (pythia.particleData.m0(had_id) - sub_mass);
    line_pts[1][0] = pythia.particleData.m0(dec_id);
    line_pts[1][1] = pythia.particleData.particleDataEntryPtr(had_id)
                         ->channel(dec_ch)
                         .bRatio();
    // calculate new branching ratio using mCP mass and polynomial approximation
    double sbRatio = poly_approx(line_pts, mCPmass);
    // exit with error if branching ratio negative
    if (sbRatio < 0) {
      cout << "Error: Calculated branching ratio approximation is < 0." << endl;
      return EXIT_FAILURE;
    }
    // set new branching ratio in pythia
    std::ostringstream strsbRatio;
    strsbRatio << sbRatio;
    pythia.readString(std::to_string(had_id) + ":" + std::to_string(dec_ch) +
                      ":bRatio = " + strsbRatio.str());
  }

  // initialize points for quadratic branching ratio approximation
  vector<vector<double>> quad_pts{
      {0., 0.},  // {0.5*(decaying hadron mass - non-muon product masses), 0}
      {pythia.particleData.m0(11), 0.},   // {m_e, e branch_ratio}
      {pythia.particleData.m0(13), 0.}};  // {m_mu, mu branch_ratio}

  // loop through decay channels and set new branching ratios for quadratic
  // approximation
  for (vector<int> ch : qchs) {
    int had_id = ch[0];
    int e_ch = ch[1];
    int mu_ch = ch[2];
    double sub_mass = 0.0;
    // loop through extra particles to subtract mass
    for (int i = 0; i < ch.size(); i++) {
      if (i >= 3) sub_mass += pythia.particleData.m0(ch[i]);
    }
    // set 0 mass then e,mu branching ratio according to IDs
    quad_pts[0][0] = 0.5 * (pythia.particleData.m0(had_id) - sub_mass);
    quad_pts[1][1] = pythia.particleData.particleDataEntryPtr(had_id)
                         ->channel(e_ch)
                         .bRatio();
    quad_pts[2][1] = pythia.particleData.particleDataEntryPtr(had_id)
                         ->channel(mu_ch)
                         .bRatio();
    // calculate new branching ratio using mCP mass and polynomial approximation
    double sbRatio = poly_approx(quad_pts, mCPmass);
    // exit with error if branching ratio negative
    if (sbRatio < 0) {
      cout << "Error: Calculated branching ratio approximation is < 0." << endl;
      return EXIT_FAILURE;
    }
    // set new branching ratio in pythia
    std::ostringstream strsbRatio;
    strsbRatio << sbRatio;
    pythia.readString(std::to_string(had_id) + ":" + std::to_string(mu_ch) +
                      ":bRatio = " + strsbRatio.str());
  }

  // change muon mass in Pythia to mCP mass
  std::ostringstream strsmCP;
  strsmCP << mCPmass;
  pythia.readString("13:m0 = " + strsmCP.str());

  // initialize pythia
  pythia.init();

  // (re)make output file, event tree
  TFile f(output_file, "recreate");
  TTree t1("EventTree", "events tree");

  // variable to store events for tree filling
  mCP_event cpevent;

  // set up TTree branches to read in
  t1.Branch("mother_id", &cpevent.mother_id, "mother_id/I");
  t1.Branch("pT", &cpevent.pT, "pT/D");
  t1.Branch("eta", &cpevent.eta, "eta/D");
  t1.Branch("ev_pTHat", &cpevent.ev_pTHat, "ev_pTHat/D");
  t1.Branch("charge", &cpevent.charge, "charge/O");
  t1.Branch("event_num", &cpevent.event_num, "event_num/i");

  // loop to generate events
  for (int iEvent = 0; iEvent < nEvent; ++iEvent) {
    // generate event, skip if error
    if (!pythia.next()) continue;

    // list of IDs of muons that came in pairs
    std::vector<int> mulist;

    // loop through particles checking daughters for muon pairs
    for (int i = 0; i < pythia.event.size(); ++i) {
      bool mu = false;
      bool mubar = false;

      // vector of possible muon pairs
      std::vector<int> poss_mupair;

      // loop through daughters to find muons
      for (int d : pythia.event[i].daughterList()) {
        // only check if final-state particles
        if (pythia.event[d].isFinal()) {
          if (pythia.event[d].id() == 13) mu = true;
          if (pythia.event[d].id() == -13) mubar = true;
          if (pythia.event[d].idAbs() == 13) poss_mupair.push_back(d);
        }
      }

      // add muons to master list if they came in a pair
      if (mu && mubar)
        for (int m : poss_mupair) mulist.push_back(m);
    }

    // set TTree variable with each muon(mCP)'s information then fill it
    for (int m : mulist) {
      if (pythia.event[m].id() > 0)  // = 13, mu
        cpevent.charge = false;      // negatively charged
      else                           // = -13, mubar
        cpevent.charge = true;       // positively charged
      cpevent.mother_id = pythia.event[pythia.event[m].mother1()].id();
      cpevent.eta = pythia.event[m].eta();
      cpevent.pT = pythia.event[m].pT();
      cpevent.ev_pTHat = pythia.info.pTHat();
      cpevent.event_num = iEvent;
      t1.Fill();

      // output found particle information
      cout << pythia.event[m].name() << ":" << endl;
      cout << "pT = " << pythia.event[m].pT() << endl;
      cout << "mother: " << pythia.event[pythia.event[m].mother1()].name()
           << endl;
      cout << "mother id = " << pythia.event[m].mother1() << endl;
    }

    // visual divider between events for output
    if (mulist.size() > 0) cout << "---------" << endl;
  }

  // output pythia stats
  pythia.stat();

  // set TTree weight to normalize to cross section and per event
  double sigma = pythia.info.sigmaGen();       // total cross section
  double weightsum = pythia.info.weightSum();  // sum of weights (# events)
  t1.SetWeight(sigma / weightsum);

  // write the tree to disk
  t1.Write();

  return EXIT_SUCCESS;
}
