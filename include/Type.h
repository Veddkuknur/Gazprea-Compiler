#ifndef TYPE_H
#define TYPE_H

#include <GazpreaBaseVisitor.h>

#include <string>
#include <vector>
#include <memory>

class ASTNode;

class Type {
public:
  enum class BaseType {
    boolean,
    character,
    integer,
    real,
    null // for tuples (they don't have a base type)
  };

  virtual ~Type() = default;
  virtual std::string getTypeAsString() = 0; // don't use this in primary implementation, only for troubleshooting
  virtual BaseType getBaseType() = 0;
  virtual void setBaseType(BaseType type) = 0;
  // methods for type-checking
  virtual bool isEqualType(std::shared_ptr<Type> type) = 0;
  // virtual std::shared_ptr<Type> promoteToType(std::shared_ptr<Type>) = 0; // need a promotion matrix somewhere
};


class PrimitiveType : public Type {
public:
  PrimitiveType() = default;
  PrimitiveType(BaseType base_type)
      : base_type(base_type) {}

  std::string getTypeAsString() override; // don't use this in primary implementation, only for troubleshooting
  BaseType getBaseType() override;
  void setBaseType(BaseType type) override;
  BaseType base_type;

  bool isEqualType(std::shared_ptr<Type> type) override;
    /* example usage:
     * PrimitiveType my_type(Type::BaseType::integer);
     * std::string str = my_type.getTypeAsString();
     */
};


class VectorType : public Type {
public:
  VectorType(BaseType base_type, std::shared_ptr<ASTNode> size)
      : base_type(base_type), size(std::move(size)) {}

  std::string getTypeAsString() override; // don't use this in primary implementation, only for troubleshooting
  BaseType getBaseType() override;
  void setBaseType(BaseType type) override;

  BaseType base_type;
  std::shared_ptr<ASTNode> size;
  bool isString  = false;
  bool isEqualType(std::shared_ptr<Type> type) override;

    /* example usage:
     * VectorType my_vector(Type::BaseType::integer, 50);
     * std::string str = my_vector.getTypeAsString();
     */
};


class MatrixType : public Type {
public:
  MatrixType(BaseType base_type, std::shared_ptr<ASTNode> rows, std::shared_ptr<ASTNode> cols)
      : base_type(base_type), rows(rows), cols(cols) {}

  std::string getTypeAsString() override; // don't use this in primary implementation, only for troubleshooting
  BaseType getBaseType() override;
  void setBaseType(BaseType type) override;

  BaseType base_type;
  std::shared_ptr<ASTNode> rows, cols;

  bool isEqualType(std::shared_ptr<Type> type) override;
    /* example usage:
     * MatrixType my_matrix(Type::BaseType::integer, 50, 20);
     * std::string str = my_vector.getTypeAsString();
     */
};


class TupleType : public Type {
public:

  // null tuple type constructor
  TupleType() = default;

  TupleType(std::vector<std::shared_ptr<Type>> element_types, std::vector<std::string> element_names)
      : element_types(std::move(element_types)), element_names(std::move(element_names)) {}

  std::string getTypeAsString() override; // don't use this in primary implementation, only for troubleshooting
  BaseType getBaseType() override;
  void setBaseType(BaseType type) override { };

  std::vector<std::shared_ptr<Type>> element_types;
  std::vector<std::string> element_names;

bool isEqualType(std::shared_ptr<Type> type) override;

/* example usage:
 * auto intType = std::make_shared<PrimitiveType>(Type::BaseType::integer);
 * auto boolType = std::make_shared<PrimitiveType>(Type::BaseType::boolean);
 * auto intMatrixType = std::make_shared<MatrixType>(Type::BaseType::integer, 50, 20);
 *
 * std::vector<std::string> name_vec{"name1", "name2", "name3"};
 *
 * std::vector<std::shared_ptr<Type>> my_element_types;
 * my_element_types.push_back(std::move(intType));
 * my_element_types.push_back(std::move(boolType));
 * my_element_types.push_back(std::move(intMatrixType));

 * TupleType my_tuple(my_element_types, name_vec);
 * std::string str = my_tuple.getTypeAsString();
 *
 */
};


#endif //TYPE_H
