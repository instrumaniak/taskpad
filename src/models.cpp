#include "models.h"

namespace taskpad {

std::string statusToString(Status s) {
  switch (s) {
    case Status::Pending: return "pending";
    case Status::InProgress: return "in_progress";
    case Status::Done: return "done";
  }
  return "pending";
}

Status stringToStatus(const std::string& s) {
  if (s == "in_progress") return Status::InProgress;
  if (s == "done") return Status::Done;
  return Status::Pending;
}

} // namespace taskpad
