// This is an automatically-generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#include <iol2opc_IDL.h>
#include <yarp/os/idl/WireTypes.h>



class iol2opc_IDL_add_object : public yarp::os::Portable {
public:
  std::string name;
  bool _return;
  void init(const std::string& name);
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

class iol2opc_IDL_remove_object : public yarp::os::Portable {
public:
  std::string name;
  bool _return;
  void init(const std::string& name);
  virtual bool write(yarp::os::ConnectionWriter& connection);
  virtual bool read(yarp::os::ConnectionReader& connection);
};

bool iol2opc_IDL_add_object::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(3)) return false;
  if (!writer.writeTag("add_object",1,2)) return false;
  if (!writer.writeString(name)) return false;
  return true;
}

bool iol2opc_IDL_add_object::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readBool(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void iol2opc_IDL_add_object::init(const std::string& name) {
  _return = false;
  this->name = name;
}

bool iol2opc_IDL_remove_object::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(3)) return false;
  if (!writer.writeTag("remove_object",1,2)) return false;
  if (!writer.writeString(name)) return false;
  return true;
}

bool iol2opc_IDL_remove_object::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListReturn()) return false;
  if (!reader.readBool(_return)) {
    reader.fail();
    return false;
  }
  return true;
}

void iol2opc_IDL_remove_object::init(const std::string& name) {
  _return = false;
  this->name = name;
}

iol2opc_IDL::iol2opc_IDL() {
  yarp().setOwner(*this);
}
bool iol2opc_IDL::add_object(const std::string& name) {
  bool _return = false;
  iol2opc_IDL_add_object helper;
  helper.init(name);
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","bool iol2opc_IDL::add_object(const std::string& name)");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool iol2opc_IDL::remove_object(const std::string& name) {
  bool _return = false;
  iol2opc_IDL_remove_object helper;
  helper.init(name);
  if (!yarp().canWrite()) {
    yError("Missing server method '%s'?","bool iol2opc_IDL::remove_object(const std::string& name)");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}

bool iol2opc_IDL::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  reader.expectAccept();
  if (!reader.readListHeader()) { reader.fail(); return false; }
  yarp::os::ConstString tag = reader.readTag();
  bool direct = (tag=="__direct__");
  if (direct) tag = reader.readTag();
  while (!reader.isError()) {
    // TODO: use quick lookup, this is just a test
    if (tag == "add_object") {
      std::string name;
      if (!reader.readString(name)) {
        reader.fail();
        return false;
      }
      bool _return;
      _return = add_object(name);
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "remove_object") {
      std::string name;
      if (!reader.readString(name)) {
        reader.fail();
        return false;
      }
      bool _return;
      _return = remove_object(name);
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "help") {
      std::string functionName;
      if (!reader.readString(functionName)) {
        functionName = "--all";
      }
      std::vector<std::string> _return=help(functionName);
      yarp::os::idl::WireWriter writer(reader);
        if (!writer.isNull()) {
          if (!writer.writeListHeader(2)) return false;
          if (!writer.writeTag("many",1, 0)) return false;
          if (!writer.writeListBegin(BOTTLE_TAG_INT, static_cast<uint32_t>(_return.size()))) return false;
          std::vector<std::string> ::iterator _iterHelp;
          for (_iterHelp = _return.begin(); _iterHelp != _return.end(); ++_iterHelp)
          {
            if (!writer.writeString(*_iterHelp)) return false;
           }
          if (!writer.writeListEnd()) return false;
        }
      reader.accept();
      return true;
    }
    if (reader.noMore()) { reader.fail(); return false; }
    yarp::os::ConstString next_tag = reader.readTag();
    if (next_tag=="") break;
    tag = tag + "_" + next_tag;
  }
  return false;
}

std::vector<std::string> iol2opc_IDL::help(const std::string& functionName) {
  bool showAll=(functionName=="--all");
  std::vector<std::string> helpString;
  if(showAll) {
    helpString.push_back("*** Available commands:");
    helpString.push_back("add_object");
    helpString.push_back("remove_object");
    helpString.push_back("help");
  }
  else {
    if (functionName=="add_object") {
      helpString.push_back("bool add_object(const std::string& name) ");
      helpString.push_back("Add a new object to the object-recognition database ");
      helpString.push_back("based on the selected blob. ");
      helpString.push_back("@param name is the object name ");
      helpString.push_back("@return true/false on success/failure. ");
    }
    if (functionName=="remove_object") {
      helpString.push_back("bool remove_object(const std::string& name) ");
      helpString.push_back("Remove object with a given name from the object-recognition ");
      helpString.push_back("database. ");
      helpString.push_back("@param name is the object name ");
      helpString.push_back("@return true/false on success/failure. ");
    }
    if (functionName=="help") {
      helpString.push_back("std::vector<std::string> help(const std::string& functionName=\"--all\")");
      helpString.push_back("Return list of available commands, or help message for a specific function");
      helpString.push_back("@param functionName name of command for which to get a detailed description. If none or '--all' is provided, print list of available commands");
      helpString.push_back("@return list of strings (one string per line)");
    }
  }
  if ( helpString.empty()) helpString.push_back("Command not found");
  return helpString;
}


