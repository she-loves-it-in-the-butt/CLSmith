#include "CLSmith/Vector.h"

#include <memory>
#include <ostream>
#include <vector>
#include <sstream>
#include <string>

#include "Block.h"
#include "CGContext.h"
#include "Constant.h"
#include "random.h"
#include "Type.h"
#include "VariableSelector.h"

class ArrayVariable;
class CVQualifiers;
class Expression;
class Variable;

namespace CLSmith {

const size_t kSizes[4] = {2, 4, 8, 16};
const char kCompSmall[4] = {'x', 'y', 'z', 'w'};
const char kCompBig[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

Vector *Vector::CreateVectorVariable(const CGContext& cg_context, Block *blk,
    const std::string& name, const Type *type, const Expression *init,
    const CVQualifiers *qfer, const Variable *isFieldVarOf) {
  assert(type != NULL);
  assert(type->eType == eSimple && type->simple_type != eVoid);
  // Vector size can only be one of four possibilities.
  int num = rnd_upto(4);
  Vector *vector = new Vector(
      blk, name, type, init, qfer, kSizes[num], isFieldVarOf);
  vector->add_init_value(Constant::make_random(type));
  blk ? blk->local_vars.push_back(vector) :
      VariableSelector::GetGlobalVariables()->push_back(vector);
  return vector;
}

Vector *Vector::itemize(void) const {
  return itemize({rnd_upto(sizes[0])});
}

Vector *Vector::itemize(const std::vector<int>& const_indices) const {
  assert(collective == NULL);
  assert(const_indices.size() == 1);
  Vector *vec = new Vector(*this);
  VariableSelector::GetAllVariables()->push_back(vec);
  vec->comp_access_.push_back(const_indices[0]);
  vec->collective = this;
  return vec;
}

Vector *Vector::itemize_simd(void) const {
  size_t num = rnd_upto(5);
  return num == 4 ? itemize() : itemize_simd(kSizes[num]);
}

Vector *Vector::itemize_simd(int access_count) const {
  assert (access_count <= 16 && access_count > 0 &&
      !(access_count & (access_count - 1)));
  std::vector<int> access;
  for (int idx = 0; idx < access_count; ++idx)
    access.push_back(rnd_upto(sizes[0]));
  return itemize_simd(access);
}

Vector *Vector::itemize_simd(const std::vector<int>& const_indices) const {
  assert(collective == NULL);
  assert(const_indices.size() != 0);
  Vector *vec = new Vector(*this);
  VariableSelector::GetAllVariables()->push_back(vec);
  vec->comp_access_ = const_indices;
  vec->collective = this;
  return vec;
}

void Vector::Output(std::ostream& out) const {
  out << get_actual_name();
  if (collective == NULL) return;
  // We have an itemised vector, output all accesses.
  out << '.';
  if (sizes[0] > 4) out << 's';
  // The components must be constant, the list of expressions used by
  // ArrayVariable cannot be used for vectors. No components means we will
  // access the whole vector, so no components need to be printed.
  for (int comp : comp_access_) out << GetComponentChar(comp);
}

void Vector::OutputDef(std::ostream& out, int indent) const {
  if (collective != NULL) return;
  output_tab(out, indent);
  OutputDecl(out);
  if (!no_loop_initializer()) {
    out << ';';
    outputln(out);
    return;
  }
  std::vector<std::string> init_strs;
  assert(init);
  init_strs.push_back(init->to_string());
  for (const auto& expr : init_values) init_strs.push_back(expr->to_string());
  std::string init_str = build_initializer_str(init_strs);
  out << " = " << build_initializer_str(init_strs) << ';';
  outputln(out);
}

void Vector::OutputDecl(std::ostream& out) const {
  // Trying to print all qualifiers prints the type as well. We don't allow
  // vector pointers regardless.
  qfer.OutputFirstQuals(out);
  OutputVectorType(out);
  out << ' ' << get_actual_name();
}

void Vector::hash(std::ostream& out) const {
  if (collective != NULL || !CGOptions::compute_hash()) return;
  // Create temporary itemised vector for printing.
  Vector *vec = itemize({0});
  for (unsigned comp = 0; comp < sizes[0]; ++comp) {
    vec->comp_access_[0] = comp;
    output_tab(out, 1);
    out << "transparent_crc(";
    vec->Output(out);
    out << ", \"";
    vec->Output(out);
    out << "\", print_hash_value);";
    outputln(out);
  }
}

std::string Vector::build_initializer_str(
    const std::vector<std::string>& init_strings) const {
  static unsigned long seed = 0xAB;
  std::string ret;
  ret.reserve(1000);
  // Print the raw vector type without qualifiers.
  stringstream ss_type;
  OutputVectorType(ss_type);
  ret.append("(").append(ss_type.str()).append(")");
  // Build a nested set of vector initialisers
  ret.append("(");
  for (unsigned idx = 0; idx < sizes[0]; ++idx) {
    unsigned long rnd_index = ((seed * seed + (idx + 7) * (idx + 13)) * 487);
    if (seed >= 0x7AB) seed = 0xAB;
    unsigned items_left = sizes[0] - idx;
    // Print a nested vector with 50% probability.
    if (items_left > kSizes[0] && rnd_upto(2) == 1) {
      int max_size_idx = 3;
      while (kSizes[max_size_idx] > items_left - 1) --max_size_idx;
      int size = kSizes[rnd_index % (max_size_idx + 1)];
      Vector vec(NULL, "", type, NULL, &qfer, size, NULL);
      ret.append(vec.build_initializer_str(init_strings));
      idx += size - 1;
    } else {
      ret.append(init_strings[rnd_index % init_strings.size()]);
    }
    if (items_left > 1) ret.append(", ");
  }
  ret.append(")");
  return ret;
}

char Vector::GetComponentChar(int index) const {
  return sizes[0] <= 4 ? kCompSmall[index] : kCompBig[index];
}

void Vector::OutputVectorType(std::ostream& out) const {
  out << "VECTOR(";
  type->Output(out);
  out << ", " << sizes[0] << ')';
}

}  // namespace CLSmith