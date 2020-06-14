#include <fstream>
#include <sstream>
#include <map>
#include "ns3/log.h"
#include "ns3/unused.h"
#include "ns3/simulator.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns2-mobility-helper.h"

namespace ns3{
NS_LOG_COMPONENT_DEFINE ("CustomHelper");

// Constants definitions
#define  NS2_AT       "at"
#define  NS2_X_COORD  "X_"
#define  NS2_Y_COORD  "Y_"
#define  NS2_Z_COORD  "Z_"
#define  NS2_SETDEST  "setdest"
#define  NS2_SET      "set"
#define  NS2_NODEID   "$node_("
#define  NS2_NS_SCH   "$ns_"

struct ParseResult
{
  std::vector<std::string> tokens; //!< tokens from a line
  std::vector<int> ivals;     //!< int values for each tokens
  std::vector<bool> has_ival; //!< points if a tokens has an int value
  std::vector<double> dvals;  //!< double values for each tokens
  std::vector<bool> has_dval; //!< points if a tokens has a double value
  std::vector<std::string> svals;  //!< string value for each token
};
  
CustomHelper::CustomHelper (std::string filename)
  : m_filename (filename)
{
  std::ifstream file (m_filename.c_str (), std::ios::in);
  if (!(file.is_open ())) NS_FATAL_ERROR("Could not open trace file " << m_filename.c_str() << " for reading, aborting here \n"); 
}

Ptr<ConstantVelocityMobilityModel>
CustomHelper::GetMobilityModel (std::string idString, const ObjectStore &store) const
{
  std::istringstream iss;
  iss.str (idString);
  uint32_t id (0);
  iss >> id;
  Ptr<Object> object = store.Get (id);
  std::cout<< "mobility id " << id << std::endl;
  if (object == 0)
    {
      return 0;
    }
  Ptr<ConstantVelocityMobilityModel> model = object->GetObject<ConstantVelocityMobilityModel> ();
  if (model == 0)
    {
      model = CreateObject<ConstantVelocityMobilityModel> ();
      object->AggregateObject (model);
    }
  return model;
}

void
CustomHelper::ConfigNodesMovements (const ObjectStore &store) const{

  file.open (m_filename.c_str (), std::ios::in);
  if (file.is_open ())
    {
       while (!file.eof () )
	 {
           std::string nodeId;
           std::string line;

           getline (file, line);

           // ignore empty lines
           if (line.empty ())
             {
               continue;
             }
	   ParseResult pr = ParseNs2Line (line); // Parse line and obtain tokens

          // Check if the line corresponds with one of the three types of line
          if (pr.tokens.size () != 4 && pr.tokens.size () != 7 && pr.tokens.size () != 8)
            {
              NS_LOG_ERROR ("Line has not correct number of parameters (corrupted file?): " << line << "\n");
              continue;
            }

          // Get the node Id
          nodeId  = GetNodeIdString (pr);
	  Ptr<ConstantVelocityMobilityModel> model = GetMobilityModel (nodeId,store);

          // if model not exists, continue
          if (model == 0)
            {
              NS_LOG_ERROR ("Unknown node ID (corrupted file?): " << nodeId << "\n");
              continue;
            }

	  Simulator(model,pr.dvals[2],pr.dvals[5],pr.dvals[6],pr.dvals[7];
	  
  
         }
    }

}

ParseResult
ParseNs2Line (const std::string& str)
{
  ParseResult ret;
  std::istringstream s;
  std::string line;

  // ignore comments (#)
  size_t pos_sharp = str.find_first_of ('#');
  if (pos_sharp != std::string::npos)
    {
      line = str.substr (0, pos_sharp);
    }
  else
    {
      line = str;
    }

  line = TrimNs2Line (line);

  // If line hasn't a correct node Id
  /* if (!HasNodeIdNumber (line))
    {
      NS_LOG_WARN ("Line has no node Id: " << line);
      return ret;
      }*/

  s.str (line);

  while (!s.eof ())
    {
      std::string x;
      s >> x;
      if (x.length () == 0)
        {
          continue;
        }
      ret.tokens.push_back (x);
      int ii (0);
      double d (0);
      if (HasNodeIdNumber (x))
        {
          x = GetNodeIdFromToken (x);
        }
      ret.has_ival.push_back (IsVal<int> (x, ii));
      ret.ivals.push_back (ii);
      ret.has_dval.push_back (IsVal<double> (x, d));
      ret.dvals.push_back (d);
      ret.svals.push_back (x);
    }

  size_t tokensLength   = ret.tokens.size ();                 // number of tokens in line
  size_t lasTokenLength = ret.tokens[tokensLength - 1].size (); // length of the last token

  // if it is a scheduled set _[XYZ] or a setdest I need to remove the last "
  // and re-calculate values
  if ( (tokensLength == 7 || tokensLength == 8)
       && (ret.tokens[tokensLength - 1][lasTokenLength - 1] == '"') )
    {
      std::cout << "impossible point" <<std::endl;
      // removes " from the last position
      ret.tokens[tokensLength - 1] = ret.tokens[tokensLength - 1].substr (0,lasTokenLength - 1);

      std::string x;
      x = ret.tokens[tokensLength - 1];

      if (HasNodeIdNumber (x))
        {
          x = GetNodeIdFromToken (x);
        }

      // Re calculate values
      int ii (0);
      double d (0);
      ret.has_ival[tokensLength - 1] = IsVal<int> (x, ii);
      ret.ivals[tokensLength - 1] = ii;
      ret.has_dval[tokensLength - 1] = IsVal<double> (x, d);
      ret.dvals[tokensLength - 1] = d;
      ret.svals[tokensLength - 1] = x;

    }
  else if ( (tokensLength == 9 && ret.tokens[tokensLength - 1] == "\"")
            || (tokensLength == 8 && ret.tokens[tokensLength - 1] == "\""))
    {
      // if the line has the " character in this way: $ns_ at 1 "$node_(0) setdest 2 2 1  "
      // or in this: $ns_ at 4 "$node_(0) set X_ 2  " we need to ignore this last token

      ret.tokens.erase (ret.tokens.begin () + tokensLength - 1);
      ret.has_ival.erase (ret.has_ival.begin () + tokensLength - 1);
      ret.ivals.erase (ret.ivals.begin () + tokensLength - 1);
      ret.has_dval.erase (ret.has_dval.begin () + tokensLength - 1);
      ret.dvals.erase (ret.dvals.begin () + tokensLength - 1);
      ret.svals.erase (ret.svals.begin () + tokensLength - 1);

    }



  return ret;
}


std::string
TrimNs2Line (const std::string& s)
{
  std::string ret = s;

  while (ret.size () > 0 && isblank (ret[0]))
    {
      ret.erase (0, 1);    // Removes blank spaces at the beginning of the line
    }

  while (ret.size () > 0 && (isblank (ret[ret.size () - 1]) || (ret[ret.size () - 1] == ';')))
    {
      ret.erase (ret.size () - 1, 1); // Removes blank spaces from at end of line
    }

  return ret;
}

template<class T>
bool IsVal (const std::string& str, T& ret)
{
  if (str.size () == 0)
    {
      return false;
    }
  else if (IsNumber (str))
    {
      std::string s2 = str;
      std::istringstream s (s2);
      s >> ret;
      return true;
    }
  else
    {
      return false;
    }
}

std::string
GetNodeIdFromToken (std::string str)
{
  if (HasNodeIdNumber (str))
    {
      // find brackets
        std::cout<<str<<std::endl;
      std::string::size_type startNodeId = str.find_first_of ("(");     // index of left bracket
      std::string::size_type endNodeId   = str.find_first_of (")");     // index of right bracket

      return str.substr (startNodeId + 1, endNodeId - (startNodeId + 1)); // set node id
    }
  else
    {
      return "";
    }
}

std::string
GetNodeIdString (ParseResult pr)
{
  switch (pr.tokens.size ())
    {
    case 4:   // line like $node_(0) set X_ 11
      return pr.svals[0];
      break;
    case 7:   // line like $ns_ at 4 "$node_(0) set X_ 28"
      return pr.svals[3];
      break;
    case 8:   // line like $ns_ at 1 "$node_(0) setdest 2 3 4"
      return pr.svals[3];
      break;
    default:
      return "";
    }
}

 
void Simulator(Ptr<ConstantVelocityMobilityModel> model, double at,
	       double xFinalPosition, double yFinalPosition, double speed)
{
  Vector position;
  position.x = xFinalPosition;
  position.y = yFinalPosition;
  position.z = 0.0;

  double xSpeed = speed*0.75;
  double ySpeed = speed*0.25;
  double zSpeed = 0.0;

  Simulator::Schedule (Seconds (at), &ConstantVelocityMobilityModel::SetVelocity, model, Vector (xSpeed, ySpeed, zSpeed));
  Simulator::Schedule (Seconds (at), &ConstantVelocityMobilityModel::SetPosition, model,position);
}

void
CustomHelper::Install (void) const
{
  Install (NodeList::Begin (), NodeList::End ());
}
  
}
