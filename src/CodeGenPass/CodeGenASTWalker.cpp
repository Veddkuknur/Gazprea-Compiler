#include "CodeGenASTWalker.h"
#include "ScopedSymbol.h"
#include "Symbol.h"


std::any CodeGenASTWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    node->global_block->accept(*this);
    // inject the runtime global declarations at the start of main procedure
    backend.insertGlobalDeclBlock();
    return {};
}




std::any CodeGenASTWalker::visitDeclNode(const std::shared_ptr<DeclNode> node) {
    const auto symbol = node->containing_scope->resolve(node->id);
    mlir::Value global_op;
    if (node->containing_scope->getScopeName() == "global") {
        // declare the global variable at the module level
        global_op = backend.declareGlobal(symbol, node->id);
        // store the global's address in the symbol table
        symbol->value = global_op;
    }
    if (node->expr == nullptr) {
        if (std::dynamic_pointer_cast<TupleType>(node->type)) {
            backend.emitCreateTuple(std::dynamic_pointer_cast<TupleType>(node->type), symbol, *this);
            return {};


        } else if (auto vec_type = std::dynamic_pointer_cast<VectorType>(node->type)){
            auto size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));
            auto vec =backend.createVector(size, symbol->type);
            symbol->value = vec;
        } else if (auto matType = std::dynamic_pointer_cast<MatrixType>(node->type)){
          auto rows = std::any_cast<mlir::Value>(matType->rows->accept(*this));
          auto cols = std::any_cast<mlir::Value>(matType->cols->accept(*this));
          auto matrix = backend.emitZeroInitMatrix(rows, cols, node->type);
          backend.emitDec(symbol, matrix);
          return {};
        }
        backend.emitDec(symbol);
        return {};
    }

    if (node->containing_scope->getScopeName() == "global") {
        mlir::Value result = std::any_cast<mlir::Value>(node->expr->accept(*this));
        if (auto vec_type = std::dynamic_pointer_cast<VectorType>(node->type)) {
            if (!backend.isStructType(result)) {
                auto size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));
                auto vec_result = backend.scalarToVector(result, size, symbol->type);
                backend.emitDecGlobal(symbol, global_op, vec_result);
                backend.setInsertionPointToModuleEnd();
                return {};
            } else if (vec_type->size) {
                auto size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));
                backend.checkDeclSizeError(size, backend.getArrSize(result));
                auto dest_vector = backend.createVector(size, symbol->type);
                backend.transferVecElements(result, dest_vector, symbol->type);
                backend.emitDecGlobal(symbol, global_op, dest_vector);
                backend.setInsertionPointToModuleEnd();
                return {};
            } else {
                backend.emitDecGlobal(symbol, global_op, result);
                backend.setInsertionPointToModuleEnd();
                return {};
            }
        } else if (auto matType = std::dynamic_pointer_cast<MatrixType>(node->type)) {
          if (auto resType = std::dynamic_pointer_cast<MatrixType>(node->expr->type)) {
            if (matType->rows != nullptr && resType->rows != nullptr) {
              resType->rows = matType->rows;
            }
            if (matType->cols != nullptr && resType->cols != nullptr) {
              resType->cols = matType->cols;
            }
          }
          if (std::dynamic_pointer_cast<PrimitiveType>((node->expr->type)) ||
              BackEnd::isCharType(result) ||
              BackEnd::isFloatType(result) ||
              BackEnd::isIntegerType(result) ||
              BackEnd::isBoolType(result)) {
            // If it is int and can be promoted:
            auto initValue = std::any_cast<mlir::Value>(node->expr->accept(*this));
            if (node->type->getBaseType() == Type::BaseType::real &&
                node->expr->type->getBaseType() == Type::BaseType::integer) {
              node->expr->type = node->type;
            }
            auto rows = std::any_cast<mlir::Value>(matType->rows->accept(*this));
            auto cols = std::any_cast<mlir::Value>(matType->cols->accept(*this));
            result = backend.emitInitializedMatrix(initValue, rows, cols, node->type);
          }

          if (node->type->getBaseType() == Type::BaseType::real &&
              node->expr->type->getBaseType() == Type::BaseType::integer) {
            auto rows = backend.emitGetRows(result);
            auto cols = backend.emitGetCols(result);
            auto resultant = backend.emitZeroInitMatrix(rows, cols, node->type);
            backend.UpCastIToFMat(resultant, result);
            backend.freeMatrix(result);
            result = resultant;
            node->expr->type = node->type;
          }

          //  Upsizing
          if (node->type->isEqualType(node->expr->type)) {
            // promoted or is same
            mlir::Value rows, cols;

            if (matType->rows) {
              rows = std::any_cast<mlir::Value>(matType->rows->accept(*this));
            } else {
              rows = backend.emitGetRows(result);
            }

            if (matType->cols) {
              cols = std::any_cast<mlir::Value>(matType->cols->accept(*this));
            } else {
              cols = backend.emitGetCols(result);
            }

            auto resultant = backend.emitZeroInitMatrix(rows, cols, node->type);
            backend.UpSizeMatrix(resultant, result, node->type);
            backend.freeMatrix(result);
            result = resultant;
          } else {
            throw TypeError(node->line, "Cannot assign a matrix of different type");
          }
          backend.emitDecGlobal(symbol, global_op, result);
          backend.setInsertionPointToModuleEnd();
          return {};
        }
        //Otherwise emit global for any other variable
        backend.emitDecGlobal(symbol, global_op, result);
        backend.setInsertionPointToModuleEnd();

    } else {
        if (std::dynamic_pointer_cast<TupleType>(node->type)) {
            if (const auto idNode = std::dynamic_pointer_cast<IdNode>(node->expr)) {
                const auto idSymbol = node->containing_scope->resolve(idNode->id);


                const mlir::Value result = backend.emitCreateTuple(std::dynamic_pointer_cast<TupleType>(symbol->type),
                                                                   backend.tupleValues(
                                                                       idSymbol));
                backend.emitDec(symbol, result);
                return {};
            }
        } else if (auto vec_type = std::dynamic_pointer_cast<VectorType>(node->type)) { //Check if the vectortype is of vec size
            auto result = std::any_cast<mlir::Value>(node->expr->accept(*this));
            if (!backend.isStructType(result)) {
                auto size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));
                auto vec_result = backend.scalarToVector(result, size, symbol->type);
                backend.emitDec(symbol, vec_result);
                return {};
            } else {
                if (vec_type->size) {
                    auto size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));
                    backend.checkDeclSizeError(size, backend.getArrSize(result));

                    auto dest_vector = backend.createVector(size, symbol->type);
                    backend.transferVecElements(result, dest_vector, symbol->type);
                    backend.emitDec(symbol, dest_vector);
                } else {
                    backend.emitDec(symbol, result);
                }
            }
            return {};
        } else if (auto matType = std::dynamic_pointer_cast<MatrixType>(node->type)){
          if (auto resType = std::dynamic_pointer_cast<MatrixType>(node->expr->type)){
            if(matType->rows != nullptr && resType->rows != nullptr){
              resType->rows = matType->rows;
            }
            if (matType->cols != nullptr && resType->cols != nullptr){
              resType->cols = matType->cols;
            }
          }
          auto result = std::any_cast<mlir::Value>(node->expr->accept(*this));
          if (std::dynamic_pointer_cast<PrimitiveType>((node->expr->type)) ||
            BackEnd::isCharType(result) ||
            BackEnd::isFloatType(result)||
            BackEnd::isIntegerType(result)||
            BackEnd::isBoolType(result)){
            // If it is int and can be promoted:
            auto initValue = std::any_cast<mlir::Value>(node->expr->accept(*this));
            if (node->type->getBaseType() == Type::BaseType::real && node->expr->type->getBaseType() == Type::BaseType::integer){
              node->expr->type = node->type;
            }
            auto rows = std::any_cast<mlir::Value>(matType->rows->accept(*this));
            auto cols = std::any_cast<mlir::Value>(matType->cols->accept(*this));
            result = backend.emitInitializedMatrix(initValue, rows, cols, node->type);
          }

          if (node->type->getBaseType() == Type::BaseType::real && node->expr->type->getBaseType() == Type::BaseType::integer){
            auto rows = backend.emitGetRows(result);
            auto cols = backend.emitGetCols(result);
            auto resultant = backend.emitZeroInitMatrix(rows, cols, node->type);
            backend.UpCastIToFMat(resultant, result);
            backend.freeMatrix(result);
            result = resultant;
            node->expr->type = node->type;
          }

          //  Upsizing
          if (node->type->isEqualType(node->expr->type)){
            // promoted or is same
            mlir::Value rows, cols;

            if (matType->rows){
              rows = std::any_cast<mlir::Value>(matType->rows->accept(*this));
            } else {
              rows = backend.emitGetRows(result);
            }

            if (matType->cols){
              cols = std::any_cast<mlir::Value>(matType->cols->accept(*this));
            } else {
              cols = backend.emitGetCols(result);
            }

            auto resultant = backend.emitZeroInitMatrix(rows, cols, node->type);
            backend.UpSizeMatrix(resultant, result, node->type);
            backend.freeMatrix(result);
            result = resultant;
          } else {
            throw TypeError(node->line, "Cannot assign a matrix of different type");
          }

          backend.emitDec(symbol, result);
          return {};
        }


        //node->expr->accept(*this);
        const auto result = std::any_cast<mlir::Value>(node->expr->accept(*this));
        backend.emitDec(symbol, result);
    }
    return {};
}

std::any CodeGenASTWalker::visitIdAssignNode(const std::shared_ptr<IdAssignNode> node) {
    auto symbol = node->containing_scope->resolve(node->id);
    auto result = std::any_cast<mlir::Value>(node->expr->accept(*this));
    if (symbol == nullptr) {
        return {};
    }
    if (auto idType = std::dynamic_pointer_cast<PrimitiveType>(symbol->type)) {
        if (idType->base_type == Type::BaseType::real && BackEnd::isIntegerType(result)) {
            result = backend.typeCast(result, Type::BaseType::real);
        }
    } else if (auto tupnode = std::dynamic_pointer_cast<TupleNode>(node->expr)) {
        std::vector<mlir::Value> tupleValues;
        for (const auto &tup_el: tupnode->tuple) {
            auto result = std::any_cast<mlir::Value>(tup_el->accept(*this));
            tupleValues.push_back(result);
        }
        symbol->value = backend.emitCreateTuple(std::dynamic_pointer_cast<TupleType>(symbol->type), tupleValues);
        return {};
    } else if (auto vec_type = std::dynamic_pointer_cast<VectorType>(symbol->type)) {

        if (std::dynamic_pointer_cast<PrimitiveType>(node->expr->type)) {
            auto size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));
            result = backend.scalarToVector(result, size, symbol->type);

        }
        if(vec_type->size) {
            auto assign_size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));
            backend.checkDeclSizeError(assign_size, backend.getArrSize(result));

        }

    } else if (auto matType = std::dynamic_pointer_cast<MatrixType>(symbol->type)) {
      auto result = std::any_cast<mlir::Value>(node->expr->accept(*this));
      if (std::dynamic_pointer_cast<PrimitiveType>((node->expr->type)) ||
          BackEnd::isCharType(result) ||
          BackEnd::isFloatType(result)||
          BackEnd::isIntegerType(result)||
          BackEnd::isBoolType(result)){
        // If it is int and can be promoted:
        auto initValue = std::any_cast<mlir::Value>(node->expr->accept(*this));
        if (symbol->type->getBaseType() == Type::BaseType::real && node->expr->type->getBaseType() == Type::BaseType::integer){
          node->expr->type = symbol->type;
        }
        auto rows = std::any_cast<mlir::Value>(matType->rows->accept(*this));
        auto cols = std::any_cast<mlir::Value>(matType->cols->accept(*this));
        result = backend.emitInitializedMatrix(initValue, rows, cols, symbol->type);
      }

      if (symbol->type->getBaseType() == Type::BaseType::real && node->expr->type->getBaseType() == Type::BaseType::integer){
        auto rows = backend.emitGetRows(result);
        auto cols = backend.emitGetCols(result);
        auto resultant = backend.emitZeroInitMatrix(rows, cols, symbol->type);
        backend.UpCastIToFMat(resultant, result);
        backend.freeMatrix(result);
        result = resultant;
        node->expr->type = symbol->type;
      }

      //  Upsizing
      if (symbol->type->isEqualType(node->expr->type)){
        // promoted or is same
        auto rows = std::any_cast<mlir::Value>(matType->rows->accept(*this));
        auto cols = std::any_cast<mlir::Value>(matType->cols->accept(*this));
        auto resultant = backend.emitZeroInitMatrix(rows, cols, symbol->type);
        backend.UpSizeMatrix(resultant, result, symbol->type);
        backend.freeMatrix(result);
        result = resultant;
      } else {
        throw TypeError(node->line, "Cannot assign a matrix of different type");
      }
      symbol->value = result;
      return {};
    }
    backend.emitAssignId(symbol, result, node->line);
    return {};
}

std::any CodeGenASTWalker::visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) {
    //auto result = node->expr_assign_to->accept(*this);
    auto result = std::any_cast<mlir::Value>(node->expr_assign_with->accept(*this));
    if (auto tupAssessNode = std::dynamic_pointer_cast<TupleAccessNode>(node->expr_assign_to)) {
        const auto symbol = node->containing_scope->resolve(tupAssessNode->tuple_id->id);
        int pos = 0;
        if (auto intNode = std::dynamic_pointer_cast<IntNode>(tupAssessNode->element)) {
            pos = intNode->val - 1;
        } else if (const auto idNode = std::dynamic_pointer_cast<IdNode>(tupAssessNode->element)) {
            std::vector<std::string> elemNameVec = std::dynamic_pointer_cast<TupleType>(symbol->type)->element_names;
            if (const auto elemPos = find(elemNameVec.begin(), elemNameVec.end(), idNode->id);
                elemPos != elemNameVec.end()) {
                // Calculate the index and cast to int
                pos = static_cast<int>(elemPos - elemNameVec.begin());
            }
        }
        if (auto idType = std::dynamic_pointer_cast<PrimitiveType>(
            std::dynamic_pointer_cast<TupleType>(symbol->type)->element_types[pos])) {
            if (idType->base_type == Type::BaseType::real && BackEnd::isIntegerType(result)) {
                result = backend.typeCast(result, Type::BaseType::real);
            }
        }
        backend.emitTupAssign(symbol, pos, result);
    } else if ( auto index_node = std::dynamic_pointer_cast<IndexNode>(node->expr_assign_to)) {
            if (std::dynamic_pointer_cast<VectorType>(index_node->collection->type)) {
                mlir::Value array;
                std::shared_ptr<Type> type;
                if (auto id_node = std::dynamic_pointer_cast<IdNode>(index_node->collection)) {
                    auto array_sym = node->containing_scope->resolve(id_node->id);
                    array = std::any_cast<mlir::Value>(array_sym->value);
                    array = backend.getVector(array); //Load the array value from the alloca.
                    type = array_sym->type;
                } else {
                    throw SyntaxError(node->line, "Invalid assign of a vector");
                }
                auto index = std::any_cast<mlir::Value>(index_node->index1->accept(*this));
                index = backend.emitSub(index, backend.emitInt(1));//Fix index
                auto value = std::any_cast<mlir::Value>(node->expr_assign_with->accept(*this));
                value = backend.typeCast(value, type->getBaseType());
                backend.storeArrVal(array, index, value, type);
            } else if (std::dynamic_pointer_cast<MatrixType>(index_node->collection->type)){
              mlir::Value matrix;
              std::shared_ptr<Type> matType;
              if (auto id_node = std::dynamic_pointer_cast<IdNode>(index_node->collection)) {
                auto sym = node->containing_scope->resolve(id_node->id);
                matrix = std::any_cast<mlir::Value>(sym->value);
                matType = sym->type;
              } else {
                throw SyntaxError(node->line, "Invalid assign of a Matrix");
              }

              auto rowIntNode = std::dynamic_pointer_cast<IntNode>(index_node->index1);
              int row = -1;
              if (rowIntNode){
                row = rowIntNode->val-1;
              }

              auto colIntNode = std::dynamic_pointer_cast<IntNode>(index_node->index2);
              int col = -1;
              if (colIntNode){
                col = colIntNode->val-1;
              }
              auto value = std::any_cast<mlir::Value>(node->expr_assign_with->accept(*this));

              backend.setMatrixIndexValue(value ,matrix, row, col, matType);
            }

    }
    return {};
}


std::any CodeGenASTWalker::visitUnpackNode(std::shared_ptr<UnpackNode> node) {
    if (auto tupleId = std::dynamic_pointer_cast<IdNode>(node->tuple)) {
        const auto symbol = node->containing_scope->resolve(tupleId->id);
        if (std::dynamic_pointer_cast<TupleType>(symbol->type)->element_types.size() != node->ivalues.size()) {
            throw AssignError(node->line, "Unpack does not have the same number of elements");
        }
        std::vector<mlir::Value> resultVec = backend.tupleValues(symbol);
        for (size_t i = 0; i < node->ivalues.size(); i++) {
            if (auto idNode = std::dynamic_pointer_cast<IdNode>(node->ivalues[i])) {
                auto sym = node->containing_scope->resolve(idNode->id);
                backend.emitAssignId(sym, resultVec[i], node->line);
            } else {
                if (auto ptr = std::dynamic_pointer_cast<TupleAccessNode>(node->ivalues[i])) {
                    const auto symbol = node->containing_scope->resolve(ptr->tuple_id->id);
                    int pos = 0;
                    if (auto intNode = std::dynamic_pointer_cast<IntNode>(ptr->element)) {
                        pos = intNode->val - 1;
                    } else if (const auto idNode = std::dynamic_pointer_cast<IdNode>(ptr->element)) {
                        std::vector<std::string> elemNameVec = std::dynamic_pointer_cast<TupleType>(symbol->type)->
                                element_names;
                        if (const auto elemPos = find(elemNameVec.begin(), elemNameVec.end(), idNode->id);
                            elemPos != elemNameVec.end()) {
                            // Calculate the index and cast to int
                            pos = static_cast<int>(elemPos - elemNameVec.begin());
                        }
                    }
                    auto result = resultVec[i];
                    if (auto idType = std::dynamic_pointer_cast<PrimitiveType>(
                        std::dynamic_pointer_cast<TupleType>(symbol->type)->element_types[pos])) {
                        if (idType->base_type == Type::BaseType::real && BackEnd::isIntegerType(result)) {
                            result = backend.typeCast(result, Type::BaseType::real);
                        }
                    }
                    backend.emitTupAssign(symbol, pos, result);
                }
            }
        }
    } else if (auto tuple = std::dynamic_pointer_cast<TupleNode>(node->tuple)) {
        if (tuple->tuple.size() != node->ivalues.size()) {
            throw AssignError(node->line, "Unpack does not have the same number of elements");
        }
        std::vector<std::shared_ptr<Symbol> > symbolsVec;
        std::vector<mlir::Value> resultVec;
        for (size_t i = 0; i < static_cast<int>(node->ivalues.size()); i++) {
            auto tupEl = tuple->tuple[i];
            auto result = std::any_cast<mlir::Value>(tupEl->accept(*this));
            auto idNode = std::dynamic_pointer_cast<IdNode>(node->ivalues[i]);
            auto symbol = node->containing_scope->resolve(idNode->id);
            if (symbol == nullptr) {
                return {};
            }
            symbolsVec.push_back(symbol);
            resultVec.push_back(result);
        }
        for (size_t i = 0; i < resultVec.size(); i++) {
            backend.emitAssignId(symbolsVec[i], resultVec[i], node->line);
        }
    }
    return {};
}


std::any CodeGenASTWalker::visitTypeDefNode(std::shared_ptr<TypeDefNode> node) {
    return {};
}


std::any CodeGenASTWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

    for (const auto& stat: node->stat_nodes)
        stat->accept(*this);
    return {};
}


std::any CodeGenASTWalker::visitVecMatNode(std::shared_ptr<VecMatNode> node) {
  if (std::dynamic_pointer_cast<VectorType>(node->type)){
    std::vector<mlir::Value> values;
    mlir::Value vector;
    if (node->is_string) {
      for (auto val : node->elements) {
        auto result = std::any_cast<mlir::Value>(val->accept(*this));
        //We already contain the type of the vector node. All we need to do now is cast it to the proper type.
        values.push_back(result);
      }
      auto vec_type = std::dynamic_pointer_cast<VectorType>(node->type);
      auto size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));
      return backend.emitVectorLiteral(values, size, node->type);
    }else {
      for (auto val : node->elements) {
        auto result = std::any_cast<mlir::Value>(val->accept(*this));
        //We already contain the type of the vector node. All we need to do now is cast it to the proper type.
        if (BackEnd::isStructType(result)){
          throw TypeError(node->line, "Cannot have a double-nested slice.");
        }
        values.push_back(result);
      }
      auto vec_type = std::dynamic_pointer_cast<VectorType>(node->type);
      auto size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));
      return backend.emitVectorLiteral(values, size, node->type);
    }

    auto vec_type = std::dynamic_pointer_cast<VectorType>(node->type);
    auto size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));

    return backend.emitVectorLiteral(values, size, node->type);
  } else if (auto matType = std::dynamic_pointer_cast<MatrixType>(node->type)){
    auto rows = std::any_cast<mlir::Value>(matType->rows->accept(*this));
    auto cols = std::any_cast<mlir::Value>(matType->cols->accept(*this));
    auto matrix = backend.emitZeroInitMatrix(rows, cols, node->type);

    // Set the values
    for (size_t i = 0; i < node->elements.size(); ++i){
      auto element = node->elements[i];
      auto val = std::any_cast<mlir::Value>(element->accept(*this));
      if (auto vec = std::dynamic_pointer_cast<VecMatNode>(element)){
        // Collection of vectors
        for (size_t j = 0; j < vec->elements.size(); ++j){
          auto element = vec->elements[j];
          auto val = std::any_cast<mlir::Value>(element->accept(*this));
          if (BackEnd::isIntegerType(val) ||
              BackEnd::isFloatType(val) ||
              BackEnd::isBoolType(val) ||
              BackEnd::isCharType(val)){
            // Store it at [row][col]
            backend.setMatrixIndexValue(val, matrix, int(i), int(j), node->type);

            // free the intermediate sub-vector
          } else {
            throw TypeError(node->line, "Cannot have a double-nested slice.");
          }
        }
      } else {
        if (!BackEnd::isStructType(val)){
          backend.setRowValues(val, matrix, int(i), node->type);
        } else {
          val = backend.promoteVectorTo(val, element->type, node->type);
          backend.copyVecToRow(matrix, val, int(i), node->type);
        }
      }
      //if(BackEnd::isStructType(val)){
      //  backend.freeVector(val);
      //}
    }
    return matrix;
  }
}


std::any CodeGenASTWalker::visitBoolNode(const std::shared_ptr<BoolNode> node) {
    return backend.emitBool(node->val);
}


std::any CodeGenASTWalker::visitCharNode(const std::shared_ptr<CharNode> node) {
    return backend.emitChar(node->val);
}


std::any CodeGenASTWalker::visitIntNode(const std::shared_ptr<IntNode> node) {
    return backend.emitInt(node->val);
}


std::any CodeGenASTWalker::visitRealNode(std::shared_ptr<RealNode> node) {
    return backend.emitReal(node->val);
}


std::any CodeGenASTWalker::visitTupleNode(std::shared_ptr<TupleNode> node) {
    std::vector<mlir::Value> tupleValues;
    auto tuple_type = std::dynamic_pointer_cast<TupleType>(node->type);
    for (auto i  = 0 ; i < node->tuple.size(); i++) {
        auto tup_el = node->tuple[i];
        auto el_type = tuple_type->element_types[i];
        auto result = std::any_cast<mlir::Value>(tup_el->accept(*this));
        if (auto vec_ty =std::dynamic_pointer_cast<VectorType>(el_type)) {
            if (!backend.isStructType(result)) {
                auto size = std::any_cast<mlir::Value>(vec_ty->size->accept(*this));
                auto vec_result = backend.scalarToVector(result, size, el_type);
                result = vec_result;
            } else {
                if (vec_ty->size) {
                    auto size = std::any_cast<mlir::Value>(vec_ty->size->accept(*this));
                    //Check for possible size errors.
                    backend.checkDeclSizeError(size, backend.getArrSize(result));
                    auto dest_vector = backend.createVector(size, vec_ty);
                    backend.transferVecElements(result, dest_vector, el_type);
                    result = dest_vector;
                }
            }
        }if (auto matType =std::dynamic_pointer_cast<MatrixType>(el_type)) {
            if (auto resType = std::dynamic_pointer_cast<MatrixType>(node->tuple[i]->type)){
            if(matType->rows != nullptr && resType->rows != nullptr){
              resType->rows = matType->rows;
            }
            if (matType->cols != nullptr && resType->cols != nullptr){
              resType->cols = matType->cols;
            }
          }
          result = std::any_cast<mlir::Value>(node->tuple[i]->accept(*this));
          if (std::dynamic_pointer_cast<PrimitiveType>((node->tuple[i]->type)) ||
            BackEnd::isCharType(result) ||
            BackEnd::isFloatType(result)||
            BackEnd::isIntegerType(result)||
            BackEnd::isBoolType(result)){
            // If it is int and can be promoted:
            auto initValue = std::any_cast<mlir::Value>(node->tuple[i]->accept(*this));
            if (node->type->getBaseType() == Type::BaseType::real && node->tuple[i]->type->getBaseType() == Type::BaseType::integer){
              node->tuple[i]->type = node->type;
            }
            auto rows = std::any_cast<mlir::Value>(matType->rows->accept(*this));
            auto cols = std::any_cast<mlir::Value>(matType->cols->accept(*this));
            result = backend.emitInitializedMatrix(initValue, rows, cols, node->type);
          }

          if (node->type->getBaseType() == Type::BaseType::real && node->tuple[i]->type->getBaseType() == Type::BaseType::integer){
            auto rows = backend.emitGetRows(result);
            auto cols = backend.emitGetCols(result);
            auto resultant = backend.emitZeroInitMatrix(rows, cols, node->type);
            backend.UpCastIToFMat(resultant, result);
            backend.freeMatrix(result);
            result = resultant;
            node->tuple[i]->type = node->type;
          }
          //  Upsizing
            mlir::Value rows, cols;
            if (matType->rows){
              rows = std::any_cast<mlir::Value>(matType->rows->accept(*this));
            } else {
              rows = backend.emitGetRows(result);
            }
            if (matType->cols){
              cols = std::any_cast<mlir::Value>(matType->cols->accept(*this));
            } else {
              cols = backend.emitGetCols(result);
            }
            auto resultant = backend.emitZeroInitMatrix(rows, cols, node->tuple[i]->type);
            backend.UpSizeMatrix(resultant, result, node->tuple[i]->type);
            backend.freeMatrix(result);
            result = resultant;
        }
        tupleValues.push_back(result);
    }
    auto type = std::dynamic_pointer_cast<TupleType>(node->type);
    return backend.emitCreateTuple(type, tupleValues);
}


std::any CodeGenASTWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {
    auto result = std::any_cast<mlir::Value>(node->expr->accept(*this));

    if (std::dynamic_pointer_cast<VectorType>(node->expr->type)) {

        if (node->op == "not") {
            return backend.emitVectorNot(result, node->type);
        }
        if (node->op == "-") {
            return backend.emitVectorUnaryMinus(result, node->type);
        }
        if (node->op == "+") {
            return result;
        }
    }



    if (node->op == "not") {
        return backend.emitUnaryNot(result);
    }
    if (node->op == "-") {
        return backend.emitUnaryMinus(result);
    }
    if (node->op == "+") {
        return result;
    }
    return result;
}


mlir::Value CodeGenASTWalker::vectorBinaryOp(mlir::Value left_side, mlir::Value right_side, std::string op, std::shared_ptr<Type> &type, int line) {

    //I know I could make this more refined but we only got  6 days I got no time to make this shit look elegant.
    try {
        if (op == "+") {
            return backend.emitVectorAdd(left_side, right_side, type);
        }
        if (op == "-") {
            return backend.emitVectorSub(left_side, right_side, type);
        }
        if (op == "*") {
            return backend.emitVectorMul(left_side, right_side, type);
        }
        if (op == "/") {
            return backend.emitVectorDiv(left_side, right_side, type);
        }
        if (op == "%") {
            return backend.emitVectorRem(left_side, right_side, type);
        }
        if (op == "^") {
            return backend.emitVectorExp(left_side, right_side, type);
        }
        if (op == "<") {
            return backend.emitVectorLessThan(left_side, right_side,type );
        }
        if (op == ">") {
            return backend.emitVectorGreaterThan(left_side, right_side, type);
        }
        if (op == "<=") {
            return backend.emitVectorLessEq(left_side, right_side, type);
        }
        if (op == ">=") {
            return backend.emitVectorGreaterEq(left_side, right_side,type);
        }
        if (op == "==") {
            return backend.emitVectorEqualTo(left_side, right_side, type);
        }
        if (op == "!=") {
            return backend.emitVectorEqualTo(left_side, right_side, type);
        } if (op == "**") {
            return backend.emitVectorDot(left_side, right_side, type);
        }
        if (op == "and") {
            return backend.emitVectorAnd(left_side, right_side, type);
        }
        if (op == "or") {
            return backend.emitVectorOr(left_side, right_side, type);
        }
        if (op == "xor") {
            return backend.emitVectorXor(left_side, right_side, type);
        }
        if (op == "||") {
            return backend.emitVectorConcat(left_side, right_side, type);
        } if (op == "by") {
            return backend.emitVectorStride(left_side, right_side, type);
        }
    } catch (...) {
        throw TypeError(line, "Incompatible types");
    }

    return {};
}


std::any CodeGenASTWalker::visitIndexNode(std::shared_ptr<IndexNode> node) {
    auto collection = std::any_cast<mlir::Value>(node->collection->accept(*this));
    if (std::dynamic_pointer_cast<VectorType>(node->collection->type)){
      if (node->index1) {
        auto index1 = std::any_cast<mlir::Value>(node->index1->accept(*this));
        index1 = backend.emitSub(index1, backend.emitInt(1)); //Subtract 1 from the index to correct it
        auto result = backend.loadArrVal(collection, index1, node->collection->type);
        backend.freeVector(collection);
        return result; //Return result for now this will change for matrices
      }
    } else {

      mlir::Value matrix;
      std::shared_ptr<Type> matType;
      if (auto id_node = std::dynamic_pointer_cast<IdNode>(node->collection)) {
        auto sym = node->containing_scope->resolve(id_node->id);
        matrix = std::any_cast<mlir::Value>(sym->value);
        matType = sym->type;
      } else {
        throw SyntaxError(node->line, "Invalid assign of a Matrix");
      }

      auto rowIntNode = std::dynamic_pointer_cast<IntNode>(node->index1);
      auto row = rowIntNode->val-1;

      auto colIntNode = std::dynamic_pointer_cast<IntNode>(node->index2);
      auto col = colIntNode->val-1;

      return backend.getMatrixIndexValue(matrix, row, col, matType);
    }

    return {};
}


std::any CodeGenASTWalker::visitRangeNode(std::shared_ptr<RangeNode> node) {

    // evaluate the lower and upper bounds
    auto lower_bound = std::any_cast<mlir::Value>(node->lower_bound->accept(*this));
    auto upper_bound = std::any_cast<mlir::Value>(node->upper_bound->accept(*this));

    // calculate the size of the new vector
    // auto one = backend.emitInt(1);
    // auto size = backend.emitSub(upper_bound, lower_bound);
    // size = backend.emitAdd(size, one);
    // auto size_ptr = backend.emitAndStoreInteger(size);
    //
    // // if the size is negative, make a vector with 0 size
    // auto zero = backend.emitInt(0);
    // auto cond = backend.emitLessEq(size, zero);
    //
    // auto ip = backend.saveInsertionPoint();
    // auto neg_size_block = backend.createBlock();
    // auto pos_size_block = backend.createBlock();
    // auto control_block = backend.createBlock();
    // auto loop_block = backend.createBlock();
    // auto continue_block = backend.createBlock();
    // backend.restoreInsertionPoint(ip);
    // backend.emitCondBranch(cond, neg_size_block, pos_size_block);
    //
    // backend.setInsertionPointToStart(neg_size_block);
    // backend.storeIntegerAtPointer(zero, size_ptr);
    // backend.emitBranch(pos_size_block);
    //
    // // loop through the range, adding the elements to the vector
    // backend.setInsertionPointToStart(pos_size_block);
    // auto final_size = backend.emitLoadInteger(size_ptr);
    // mlir::Value range_vector = backend.createVector(final_size, node->type);
    // auto index = backend.emitIterLoopIndex();
    // backend.emitBranch(control_block);
    //
    // backend.setInsertionPointToStart(control_block);
    // auto index_val = backend.emitLoadIndex(index);
    // cond = backend.emitLessThan(index_val, final_size);
    // backend.emitCondBranch(cond, loop_block, continue_block);
    //
    // backend.setInsertionPointToStart(loop_block);
    // auto elem_val = backend.emitAdd(index_val, lower_bound);
    // backend.storeArrVal(range_vector, index_val, elem_val, node->type);
    //
    // auto index_plus1 = backend.emitIncrementIndex(index_val);
    // backend.emitStoreIndex(index, index_plus1);
    // backend.emitBranch(control_block);
    //
    // backend.setInsertionPointToStart(continue_block);

    auto range_vector = backend.emitRangeVector(upper_bound, lower_bound);

    return range_vector;
}



std::any CodeGenASTWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {
  auto leftType = std::dynamic_pointer_cast<MatrixType>(node->l_expr->type);
  auto rightType = std::dynamic_pointer_cast<MatrixType>(node->r_expr->type);
  if (leftType && rightType){
    if (node->op == "**"){
      auto left = std::any_cast<mlir::Value>(node->l_expr->accept(*this));
      auto right = std::any_cast<mlir::Value>(node->r_expr->accept(*this));
      mlir::Value option = (leftType->getBaseType() == Type::BaseType::integer && rightType->getBaseType() == Type::BaseType::integer)? backend.emitChar('i'): backend.emitChar('f');
      bool promote = !(leftType->getBaseType() == Type::BaseType::integer && rightType->getBaseType() == Type::BaseType::integer);
      if (promote){
        if(leftType->getBaseType() == Type::BaseType::integer){
          auto rows = backend.emitGetRows(left);
          auto cols = backend.emitGetCols(left);
          auto resultant = backend.emitZeroInitMatrix(rows, cols, node->type);
          backend.UpCastIToFMat(resultant, left);
          backend.freeMatrix(left);
          left = resultant;
        }

        if (rightType->getBaseType() == Type::BaseType::integer){
          auto rows = backend.emitGetRows(right);
          auto cols = backend.emitGetCols(right);
          auto resultant = backend.emitZeroInitMatrix(rows, cols, node->type);
          backend.UpCastIToFMat(resultant, right);
          backend.freeMatrix(right);
          right = resultant;
        }
      }

      auto resultant = backend.callMatmul(left, right, option);
      return resultant;
    } else if (node->op == "+"){
      auto left = std::any_cast<mlir::Value>(node->l_expr->accept(*this));
      auto right = std::any_cast<mlir::Value>(node->r_expr->accept(*this));
      mlir::Value option = (leftType->getBaseType() == Type::BaseType::integer && rightType->getBaseType() == Type::BaseType::integer)? backend.emitChar('i'): backend.emitChar('f');
      bool promote = !(leftType->getBaseType() == Type::BaseType::integer && rightType->getBaseType() == Type::BaseType::integer);
      if (promote){
        if(leftType->getBaseType() == Type::BaseType::integer){
          auto rows = backend.emitGetRows(left);
          auto cols = backend.emitGetCols(left);
          auto resultant = backend.emitZeroInitMatrix(rows, cols, node->type);
          backend.UpCastIToFMat(resultant, left);
          backend.freeMatrix(left);
          left = resultant;
        }

        if (rightType->getBaseType() == Type::BaseType::integer){
          auto rows = backend.emitGetRows(right);
          auto cols = backend.emitGetCols(right);
          auto resultant = backend.emitZeroInitMatrix(rows, cols, node->type);
          backend.UpCastIToFMat(resultant, right);
          backend.freeMatrix(right);
          right = resultant;
        }
      }

      auto resultant = backend.callMatAdd(left, right, option);
      return resultant;
    }
  }
    mlir::Value left_side;
    mlir::Value right_side;
    std::shared_ptr<Type> left_type;
    std::shared_ptr<Type> right_type;
    if (auto leftIdNode = std::dynamic_pointer_cast<IdNode>(node->l_expr)) {
        left_type = node->containing_scope->resolve(leftIdNode->id)->type;
        left_side = std::any_cast<mlir::Value>(node->l_expr->accept(*this));
    } else if (auto leftTupleNode = std::dynamic_pointer_cast<TupleNode>(node->l_expr)) {
        left_type = leftTupleNode->type;
        left_side = std::any_cast<mlir::Value>(node->l_expr->accept(*this));
    } else {
        left_side = std::any_cast<mlir::Value>(node->l_expr->accept(*this));
    }
    if (auto rightIdNode = std::dynamic_pointer_cast<IdNode>(node->r_expr)) {
        right_type = node->containing_scope->resolve(rightIdNode->id)->type;
        right_side = std::any_cast<mlir::Value>(node->r_expr->accept(*this));
    } else if (auto rightTupleNode = std::dynamic_pointer_cast<TupleNode>(node->r_expr)) {
        right_type = rightTupleNode->type;
        right_side = std::any_cast<mlir::Value>(node->r_expr->accept(*this));
    } else {
        right_side = std::any_cast<mlir::Value>(node->r_expr->accept(*this));
    }
    if (node->op == "||"){
        if (!BackEnd::isStructType(left_side)) {
            left_side = backend.scalarToVector(left_side,backend.emitInt(1) , node->type);
        }
        if (!BackEnd::isStructType(right_side)) {
            right_side = backend.scalarToVector(right_side, backend.emitInt(1), node->type);
        }
        return backend.emitVectorConcat(left_side, right_side,node->type);
    } if (node->op == "by") {

        return vectorBinaryOp(left_side, right_side, node->op, node->type, node->line); //If vector is by no need to do any casting.
    }
    //What do we do if we find a primitive and a vector. Do we do the type promotion inside or outside?
    if ( (std::dynamic_pointer_cast<VectorType>(node->type) && node->type->getBaseType() != Type::BaseType::boolean) ) {
        auto vec_type = std::dynamic_pointer_cast<VectorType>(node->type);
        //Check if any of the elements is a primitive
        mlir::Value size;
        //Check if the vector has the size inferred, if not then this means that the size couldn't be transfered from the declaration, which means its a var x = ....
        if (!vec_type->size) {
            if (backend.isStructType(left_side)){
                size = backend.getArrSize(left_side);
            } else if (backend.isStructType(right_side)) {
                size = backend.getArrSize(right_side);
            }
            //If the result of the operation does not have a size then we infer the size yet again
        } else {
            size = std::any_cast<mlir::Value>(vec_type->size->accept(*this));
        }
        if (!backend.isStructType(left_side)) {
            left_side = backend.scalarToVector(left_side, size, node->type);
        } else  {
            //Promote the vector to the result type
            left_side = backend.promoteVectorTo(left_side, node->l_expr->type, node->type);

        }
        if (!backend.isStructType(right_side)) {
            right_side = backend.scalarToVector(right_side, size, node->type);
        } else {
            //Promote the vector to the result type
            right_side = backend.promoteVectorTo(right_side, node->r_expr->type, node->type);

        }
        return vectorBinaryOp(left_side, right_side, node->op, node->type, node->line);
        //promote values to
        //Make
        //I can either promote both sides inside of backend or outside of backend.
    } if ((std::dynamic_pointer_cast<VectorType>(node->l_expr->type) ||
        std::dynamic_pointer_cast<VectorType>(node->r_expr->type))&&
        ((node->type->getBaseType() == Type::BaseType::boolean) xor node->op == "**") ) {

        //I know this is extremely janky but we need to be feature complete and i can't think of a more elegant solution on the spot
        auto base_type = backend.promoteBaseType(node->l_expr->type, node->r_expr->type);

        if (base_type == Type::BaseType::null) {

            throw TypeError(node->line, "Comparison between incompatible types");
        }

        auto l_type = std::dynamic_pointer_cast<VectorType>(node->l_expr->type);
        std::shared_ptr<Type> promote_result_type = std::make_shared<VectorType>(base_type, l_type->size);

        left_side = backend.promoteVectorTo(left_side, node->l_expr->type, promote_result_type);
        right_side = backend.promoteVectorTo(right_side, node->r_expr->type, promote_result_type);


        return vectorBinaryOp(left_side, right_side, node->op, promote_result_type, node->line);

    }
    // We create our method called Vecto




    //Okay, how do we do addition.
    //For vector additions there are two options. The right and left side are either an integer or a vector.
    // How do we upcast an integer, float, char, boolean. //Booleans we don't need to worry about.
    //
    // We know whatever the result of the operation is, it must be able to be upcasted to the decl.
    //
    // It will be convenient if every element of a given vector is synced with the highest possible type on its rows.
    //
    // Very similar to the upcasting done in BinaryOpError walker.
    //Vector case is not hard.

    try {
        if (node->op == "+") {
            return backend.emitAdd(left_side, right_side);
        }
        if (node->op == "-") {
            return backend.emitSub(left_side, right_side);
        }
        if (node->op == "*") {
            return backend.emitMul(left_side, right_side);
        }
        if (node->op == "/") {
            return backend.emitDiv(left_side, right_side);
        }
        if (node->op == "%") {
            return backend.emitRem(left_side, right_side);
        }
        if (node->op == "^") {
            return backend.emitExpo(left_side, right_side);
        }
        if (node->op == "<") {
            return backend.emitLessThan(left_side, right_side);
        }
        if (node->op == ">") {
            return backend.emitGreaterThan(left_side, right_side);
        }
        if (node->op == "<=") {
            return backend.emitLessEq(left_side, right_side);
        }
        if (node->op == ">=") {
            return backend.emitGreaterEq(left_side, right_side);
        }
        if (node->op == "==") {
            if (std::dynamic_pointer_cast<TupleType>(left_type) && std::dynamic_pointer_cast<TupleType>(right_type)) {
                return backend.emitEqualTo(backend.tupleValues(left_side,
                                                               std::dynamic_pointer_cast<TupleType>(
                                                                   std::dynamic_pointer_cast<TupleType>(left_type))),
                                           backend.tupleValues(right_side,
                                                               std::dynamic_pointer_cast<TupleType>(
                                                                   std::dynamic_pointer_cast<TupleType>(right_type))
                                           ), std::dynamic_pointer_cast<TupleType>(left_type),
                                           std::dynamic_pointer_cast<TupleType>(right_type));
            }
            return backend.emitEqualTo(left_side, right_side);
        }
        if (node->op == "!=") {
            if (std::dynamic_pointer_cast<TupleType>(left_type) && std::dynamic_pointer_cast<TupleType>(right_type)) {
                auto result = backend.emitEqualTo(backend.tupleValues(left_side,
                                                               std::dynamic_pointer_cast<TupleType>(
                                                                   std::dynamic_pointer_cast<TupleType>(left_type))),
                                           backend.tupleValues(right_side,
                                                               std::dynamic_pointer_cast<TupleType>(
                                                                   std::dynamic_pointer_cast<TupleType>(right_type))
                                           ), std::dynamic_pointer_cast<TupleType>(left_type),
                                           std::dynamic_pointer_cast<TupleType>(right_type));
                return backend.emitUnaryNot(result);
            }
            return backend.emitNotEqual(left_side, right_side);
        }
        if (node->op == "and") {
            return backend.emitAnd(left_side, right_side);
        }
        if (node->op == "or") {
            return backend.emitOr(left_side, right_side);
        }
        if (node->op == "xor") {
            return backend.emitXor(left_side, right_side);
        }
        if (node->op == "||") {

        }
    } catch (...) {
        throw TypeError(node->line, "Incompatible types");
    }
    return {};
}



std::any CodeGenASTWalker::visitFilterNode(std::shared_ptr<FilterNode> node) {
    //So for each predicate expression, create a filter in the backend.

    std::cout << "Inside filter node";
    auto domain_vector = std::any_cast<mlir::Value>(node->vec_expr->accept(*this));
    std::cout << "Obtained the domain address" << std::endl;
    std::vector<mlir::Value> filter_results;
    //For each predicate
    std::shared_ptr<Symbol> id_sym;

    if (auto id_node = std::dynamic_pointer_cast<IdNode>(node->id)) {
        id_sym = node->containing_scope->resolve(id_node->id);
        backend.emitDec(id_sym);
    }

    //backend.createVector(backend.emitInt(420), node->vec_expr->type);

    for (auto pred_node : node->pred_exprs){

        auto old_insertion_point = backend.saveInsertionPoint();
        auto predicate_block = new mlir::Block();
        backend.setInsertionPointToStart(predicate_block);
        //Create new block and set insertion point to the new block


        auto pred_expr = std::any_cast<mlir::Value>(pred_node->accept(*this));
        //Evaluate the predicate.
        backend.restoreInsertionPoint(old_insertion_point);
        //Call the filter node and create a filter.

        auto filter_addr = backend.createFilter(domain_vector, predicate_block, id_sym, pred_expr, node->vec_expr->type);
        //Filter spits out a vector.


        filter_results.push_back(filter_addr);


        //Add it to the array of tuple elements.

    }
    //backend.printVector(filter_results[0], node->vec_expr->type);
    //tup_type->element_types.pop_back();


    //How do we obtain the final array?
    auto old_insertion_point = backend.saveInsertionPoint();

    //Iinitialize new block
    auto last_block = new mlir::Block();
    backend.setInsertionPointToStart(last_block);

    auto result = std::any_cast<mlir::Value>(node->pred_exprs[0]->accept(*this));
    for (int i = 1 ; i < node->pred_exprs.size(); i++) {
        auto pred_expr = std::any_cast<mlir::Value>(node->pred_exprs[i]->accept(*this));    //Add each predicate expression.

        result = backend.emitOr(result, pred_expr);    //Logical OR the result for each predicate expression.


    }

    result = backend.emitUnaryNot(result);    //Then at the end negate the compounded logical expression.


    backend.restoreInsertionPoint(old_insertion_point);

    auto final_vector = backend.createFilter(domain_vector, last_block, id_sym, result, node->vec_expr->type);




    filter_results.push_back(final_vector);
    //Now check if the filter node has any types that
    auto tup_type = std::dynamic_pointer_cast<TupleType>(node->type);
    for (int i = 0; i < tup_type->element_types.size(); i++) {
        auto el_type = tup_type->element_types[i];
        if (std::dynamic_pointer_cast<VectorType>(el_type)->size) { //If the element type already has a pre-defined size then
            //If the vector already has a pre-defined size then
            auto size = std::any_cast<mlir::Value>(std::dynamic_pointer_cast<VectorType>(el_type)->size->accept(*this));
            backend.checkDeclSizeError(size, backend.getArrSize(filter_results[i]));
            auto dest_vector = backend.createVector(size, el_type);
            backend.transferVecElements(filter_results[i], dest_vector, el_type);
            filter_results[i] = dest_vector;
        }
    }
    //backend.printVector(final_vector, node->vec_expr->type);
    //Convert to tuple..



    //auto symbol = node->containing_scope->resolve(node->id);
    //backend.DeclId(symbol, backend.loadInt(0));

    //auto old_insertion_point = backend.saveInsertionPoint();

    //auto predicate_block = new mlir::Block();
    //backend.setInsertionPointToStart(predicate_block);

    //std::cout << "Resolved the id"  << std::endl;
    //auto predicate_to_eval = std::any_cast<mlir::Value>(visit(node->expr_node));
    //backend.restoreInsertionPoint(old_insertion_point);
    //std::cout << "Obtained predicate"  << std::endl;
    //auto filter_addr = backend.createFilter(vector_addr, predicate_block, symbol, predicate_to_eval);
    //return filter_addr;


    //How do I obtain the nodes that aren't in my predicate expression?
    return backend.emitCreateTuple(std::dynamic_pointer_cast<TupleType>(node->type), filter_results);

}

std::any CodeGenASTWalker::visitGeneratorNode(std::shared_ptr<GeneratorNode> node) {

    //We will have a for loop

    //For every value in the loop
    //Create a result that is an array
    //Set the ID as the current value in the domain node
    std::cout << "INside gen node";
    //auto vector_addr = std::any_cast<mlir::Value>(visit(node->domain_node));
    //std::cout << "Obtained the domain address"  << std::endl;
    mlir::Value vector_addr1;
    mlir::Value vector_addr2;

    mlir::Value id1;
    mlir::Value id2;
    std::shared_ptr<Symbol> id1_sym;
    std::shared_ptr<Symbol> id2_sym;
    std::shared_ptr<Type> vec1_type;
    std::shared_ptr<Type> vec2_type;
    if (node->vec_expr1) {
        //Visit an obtain the elements of the first domain
        vector_addr1 = std::any_cast<mlir::Value>(node->vec_expr1->accept(*this));
        vec1_type = node->vec_expr1->type;
    }


    if (node->vec_expr2) {
        //Visit the results of the second domain
        vector_addr2 = std::any_cast<mlir::Value>(node->vec_expr2->accept(*this));
        vec2_type = node->vec_expr2->type;

    }
    if (auto id1_node = std::dynamic_pointer_cast<IdNode>(node->id1)) {

        id1_sym = node->containing_scope->resolve(id1_node->id);
        backend.emitDec(id1_sym);

    }
    if (auto id2_node = std::dynamic_pointer_cast<IdNode>(node->id2)) {
        id2_sym = node->containing_scope->resolve(id2_node->id);
        backend.emitDec(id2_sym);

    }

    //Save old insertion point
    auto old_insertion_point = backend.saveInsertionPoint();
    //Create block

    auto expr_block = new mlir::Block();
    backend.setInsertionPointToStart(expr_block);

    auto expr_to_eval = std::any_cast<mlir::Value>(node->rhs_expr->accept(*this));

    backend.restoreInsertionPoint(old_insertion_point);


    if (node->vec_expr1 && ! node->vec_expr2) {
        auto gen_addr = backend.createVectorGenerator(vector_addr1, expr_block, id1_sym, expr_to_eval,
            node->type, vec1_type, node->line);
        return gen_addr;
    }



    if (node->vec_expr1 && node->vec_expr2) {
        auto mat_gen_addr = backend.createMatrixGenerator(vector_addr1, vector_addr2,  expr_block, id1_sym, id2_sym,
            expr_to_eval, node->type, vec1_type, vec2_type, node->line);
        return mat_gen_addr;
    } else {
        throw TypeError(node->line, "Oops, something went wrong omegalul");
    }




    //Check whether if generator is empty, if so return empty vector right away

    //Otherwise declare the value with the first element of domain
    //auto symbol = node->containing_scope->resolve(node->id);
    //backend.DeclId(symbol, backend.loadInt(0));


    //Save old insertion point
    /*auto old_insertion_point = backend.saveInsertionPoint();
    //Create block

    auto expr_block = new mlir::Block();
    backend.setInsertionPointToStart(expr_block);

    std::cout << "Resolved the id"  << std::endl;
    //Insert all new operations in block
    //auto expr_to_eval = std::any_cast<mlir::Value>(visit(node->expr_node));
    backend.restoreInsertionPoint(old_insertion_point);
    std::cout << "Obtained expression"  << std::endl;
    //Pass the block into createGenerator

    auto gen_addr = backend.createGenerator(vector_addr, expr_block, symbol, expr_to_eval); */
    return {};

    //Evaluate the expression of the domain node
    //auto value = std:visit(node->expr_node);
    //Expression node will return a value, this value will
    //Repeat.



}




std::any CodeGenASTWalker::visitTypeCastNode(const std::shared_ptr<TypeCastNode> node) {
    if (auto toType = std::dynamic_pointer_cast<PrimitiveType>(node->cast_type)) {
        const auto result = std::any_cast<mlir::Value>(node->expr->accept(*this));
        return backend.typeCast(result, toType->base_type);
    }
    if ( auto toType = std::dynamic_pointer_cast<TupleType>(node->cast_type)) {
        if (const auto idNode = std::dynamic_pointer_cast<IdNode>(node->expr)) {
            std::shared_ptr<Symbol> symbol = node->containing_scope->resolve(idNode->id);
            auto structPtr = std::any_cast<mlir::Value>(symbol->value);
            if (symbol->scope->getScopeName() == "global") {
                structPtr = backend.emitLoadID(symbol, symbol->name);
            }
            return backend.emitCastTuple(toType, std::dynamic_pointer_cast<TupleType>(symbol->type),
                                         structPtr, *this);
        }
        if (const auto tupleNode = std::dynamic_pointer_cast<TupleNode>(node->expr)) {
            std::vector<mlir::Value> tupleValues;
            auto tuple_expr = std::dynamic_pointer_cast<TupleNode>(node->expr)->tuple;
            for (auto i = 0; i < tuple_expr.size(); i++ ) {
                auto tup_el = tuple_expr[i];
                auto result = std::any_cast<mlir::Value>(tup_el->accept(*this));
                //This is the quickest way I could think about doing this may god forgive me for this.
                if (auto vec_type = std::dynamic_pointer_cast<VectorType>(toType->element_types[i])) {

                    result = castVector(result, tup_el->type, toType->element_types[i]);

                }
                tupleValues.push_back(result);
            }
            return backend.emitCreateTuple(toType, tupleValues);
        }
    } else if ( auto toType = std::dynamic_pointer_cast<VectorType>(node->cast_type)) {

        auto result = std::any_cast<mlir::Value>(node->expr->accept(*this));
        if (std::dynamic_pointer_cast<PrimitiveType>(node->expr->type)) {
            auto size = std::any_cast<mlir::Value>(toType->size->accept(*this));
            result = backend.scalarToVector(result, size, node->cast_type);
            return result;
        } else if (std::dynamic_pointer_cast<VectorType>(node->expr->type)) {
            return castVector(result, node->expr->type, node->cast_type);
        }

    }
    return {};
}


mlir::Value CodeGenASTWalker::castVector(mlir::Value vector_addr, std::shared_ptr<Type> &vec_type, std::shared_ptr<Type> &to_type) {

    auto toType = std::dynamic_pointer_cast<VectorType>(to_type);
    if (std::dynamic_pointer_cast<PrimitiveType>(vec_type)) {
        auto size = std::any_cast<mlir::Value>(toType->size->accept(*this));
        vector_addr = backend.scalarToVector(vector_addr, size, to_type);
        return vector_addr;
    } else if (std::dynamic_pointer_cast<VectorType>(vec_type)) {
        if (toType->size) {
            auto size = std::any_cast<mlir::Value>(toType->size->accept(*this));
            vector_addr = backend.promoteVectorTo(vector_addr, vec_type, to_type);
            auto dest_vec = backend.createVector(size, toType);
            backend.transferVecElements(vector_addr, dest_vec, to_type);
            return dest_vec;
        } else {
            vector_addr = backend.promoteVectorTo(vector_addr, vec_type, to_type);
            return vector_addr;
        }
    }


}


std::any CodeGenASTWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
    auto ip = backend.saveInsertionPoint();
    auto loop_block = backend.createBlock();
    auto continue_block = backend.createBlock();
    backend.restoreInsertionPoint(ip);

    // set condition branch whose control expression is always true
    auto condition = backend.emitBool(true);
    backend.emitCondBranch(condition, loop_block, continue_block);
    backend.setInsertionPointToStart(loop_block);

    mlir::Value stack_ptr = backend.emitCurrentStackPointer();

    // store this loop's body block and continue block on the stack for breaks/continues to know where to jump
    backend.nested_blocks_stack.emplace_back(loop_block, continue_block, stack_ptr, nullptr);
    node->loop_block->accept(*this);
    backend.nested_blocks_stack.pop_back();

    // if the loop block doesn't have a terminal op in it, then this loop does not nest another cond or loop
    backend.setInsertionPointToEnd(loop_block);
    if (!backend.hasTerminator(loop_block)) {
        backend.emitRestoreStackPointer(stack_ptr);
        backend.emitBranch(loop_block);
    }
    // check the validity of the last inner continue block (eliminate terminal ops that aren't reached)
    if (backend.last_inner_continue_block != nullptr) {
        // check if the second-to-last op is terminal
        if (backend.last_inner_continue_block->getOperations().size() > 2) {
            // Erase the last 2 ops if third-to-last op is terminal.
            auto &third_last_op = *std::prev(backend.last_inner_continue_block->end(), 3);
            if (backend.isTerminator(&third_last_op)) {
                backend.last_inner_continue_block->back().erase();
                backend.last_inner_continue_block->back().erase();
            }
            if (backend.last_inner_continue_block->getOperations().size() > 1) {
                // Erase the last op if second-to-last op is terminal.
                auto &second_last_op = *std::prev(backend.last_inner_continue_block->end(), 2);
                if (backend.isTerminator(&second_last_op)) {
                    backend.last_inner_continue_block->back().erase();
                }
            }
        }
        backend.last_inner_continue_block = nullptr;
    }
    // check if we are nested inside another conditional or loop and if so:
    // branch to the appropriate block (depending on if its from a loop or cond)
    backend.setInsertionPointToStart(continue_block);
    if (!backend.nested_blocks_stack.empty()) {
        // check if block pair has a back_edge (loop)
        mlir::Block* block_to_branch_to;
        if (std::get<0>(backend.nested_blocks_stack.back()) != nullptr) {
            block_to_branch_to = std::get<0>(backend.nested_blocks_stack.back());
            mlir::Value old_sp = std::get<2>(backend.nested_blocks_stack.back());
            backend.emitRestoreStackPointer(old_sp);
        }
        else
            block_to_branch_to = std::get<1>(backend.nested_blocks_stack.back());
        backend.emitBranch(block_to_branch_to); // in case current continue block doesn't emit a terminal op
        backend.last_inner_continue_block = continue_block;
        // save this block in case we need to remove the branch instruction later
    }
    // populate the continue block
    backend.setInsertionPointToStart(continue_block);
    return {};
}


std::any CodeGenASTWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
    auto ip = backend.saveInsertionPoint();
    auto control_block = backend.createBlock();
    auto loop_block = backend.createBlock();
    auto continue_block = backend.createBlock();

    // enter loop_block or control_block first depending on if the loop is pre_predicated
    backend.restoreInsertionPoint(ip);
    if (node->pre_predicated)
        backend.emitBranch(control_block);
    else
        backend.emitBranch(loop_block);

    // emit the control expression and conditional branch
    backend.setInsertionPointToStart(control_block);
    auto cond = std::any_cast<mlir::Value>(node->contr_expr->accept(*this));
    backend.emitCondBranch(cond, loop_block, continue_block);

    // populate the loop block
    backend.setInsertionPointToStart(loop_block);

    // push the stack pointer on the stack pointer stack
    mlir::Value stack_ptr = backend.emitCurrentStackPointer();

    // store this loop's control_block and continue block on the stack for breaks/continues to know where to jump
    backend.nested_blocks_stack.emplace_back(control_block, continue_block, stack_ptr, nullptr);
    node->loop_block->accept(*this);
    backend.nested_blocks_stack.pop_back();

    // if the loop block doesn't have a terminal op in it, then this loop does not nest another cond or loop
    backend.setInsertionPointToEnd(loop_block);
    if (!backend.hasTerminator(loop_block)) {
        backend.emitRestoreStackPointer(stack_ptr);
        backend.emitBranch(control_block);
    }
    // check the validity of the last inner continue block (eliminate terminal ops that aren't reached)
    if (backend.last_inner_continue_block != nullptr) {
        if (backend.last_inner_continue_block->getOperations().size() > 2) {
            // Erase the last 2 ops if third-to-last op is terminal.
            auto &third_last_op = *std::prev(backend.last_inner_continue_block->end(), 3);
            if (backend.isTerminator(&third_last_op)) {
                backend.last_inner_continue_block->back().erase();
                backend.last_inner_continue_block->back().erase();
            }
            if (backend.last_inner_continue_block->getOperations().size() > 1) {
                // Erase the last op if second-to-last op is terminal.
                auto &second_last_op = *std::prev(backend.last_inner_continue_block->end(), 2);
                if (backend.isTerminator(&second_last_op)) {
                    backend.last_inner_continue_block->back().erase();
                }
            }
        }
        backend.last_inner_continue_block = nullptr;
    }
    // check if we are nested inside another conditional or loop and if so:
    // branch to the appropriate block (depending on if its from a loop or cond)
    backend.setInsertionPointToStart(continue_block);
    if (!backend.nested_blocks_stack.empty()) {
        // check if block pair has a back_edge (loop)
        mlir::Block* block_to_branch_to;
        if (std::get<0>(backend.nested_blocks_stack.back()) != nullptr) {
            block_to_branch_to = std::get<0>(backend.nested_blocks_stack.back());
            mlir::Value old_sp = std::get<2>(backend.nested_blocks_stack.back());
            backend.emitRestoreStackPointer(old_sp);
        }
        else
            block_to_branch_to = std::get<1>(backend.nested_blocks_stack.back());
        backend.emitBranch(block_to_branch_to); // in case current continue block doesn't emit a terminal op
        backend.last_inner_continue_block = continue_block;
        // save this block in case we need to remove the branch instruction later
    }
    // populate the continue block
    backend.setInsertionPointToStart(continue_block);
    return {};
}


std::any CodeGenASTWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
    auto ip = backend.saveInsertionPoint();
    auto element_access_block = backend.createBlock();
    auto loop_block = backend.createBlock();
    auto continue_block = backend.createBlock();
    backend.restoreInsertionPoint(ip);

    // evaluate the vector in the domain expression, and get its size
    auto domain_vector = std::any_cast<mlir::Value>(node->dom_expr->accept(*this));
    auto vector_size = backend.getArrSize(domain_vector, true);

    // create the vector index value
    auto index = backend.emitIterLoopIndex();

    // create the domain variable
    auto vec_base_type = backend.getMLIRTypeFromBaseType(node->dom_expr->type);
    auto dom_var = backend.emitDomainVariable(vec_base_type);

    // store the domain variable value in the scope table
    auto dom_var_symbol = node->loop_block->containing_scope->resolve(node->dom_id);
    dom_var_symbol->value = dom_var;

    backend.emitBranch(element_access_block);
    backend.setInsertionPointToStart(element_access_block);

    // if the index is less than the size of the vector, goto loop block, else goto continue block
    auto index_val = backend.emitLoadIndex(index);
    auto index_val_plus1 = backend.emitIncrementIndex(index_val);
    auto cond = backend.emitLessThan(index_val, vector_size);
    backend.emitStoreIndex(index, index_val_plus1);
    backend.emitCondBranch(cond, loop_block, continue_block);

    // populate the loop block
    backend.setInsertionPointToStart(loop_block);

    // push the stack pointer on the stack pointer stack
    mlir::Value stack_ptr = backend.emitCurrentStackPointer();

    // assign the (index_value)th element value to the domain variable
    auto dv_value = backend.loadArrVal(domain_vector, index_val, node->dom_expr->type);
    backend.emitStoreDomainVariable(dom_var, dv_value);

    // store this loop's control_block and continue block on the stack for breaks/continues to know where to jump
    backend.nested_blocks_stack.emplace_back(element_access_block, continue_block, stack_ptr, domain_vector);
    node->loop_block->accept(*this);
    backend.nested_blocks_stack.pop_back();

    // if the loop block doesn't have a terminal op in it, then this loop does not nest another cond or loop
    backend.setInsertionPointToEnd(loop_block);
    if (!backend.hasTerminator(loop_block)) {
        backend.emitRestoreStackPointer(stack_ptr);
        backend.emitBranch(element_access_block);
    }
    // check the validity of the last inner continue block (eliminate terminal ops that aren't reached)
    if (backend.last_inner_continue_block != nullptr) {
        if (backend.last_inner_continue_block->getOperations().size() > 2) {
            // Erase the last 2 ops if third-to-last op is terminal.
            auto &third_last_op = *std::prev(backend.last_inner_continue_block->end(), 3);
            if (backend.isTerminator(&third_last_op)) {
                backend.last_inner_continue_block->back().erase();
                backend.last_inner_continue_block->back().erase();
            }
            if (backend.last_inner_continue_block->getOperations().size() > 1) {
                // Erase the last op if second-to-last op is terminal.
                auto &second_last_op = *std::prev(backend.last_inner_continue_block->end(), 2);
                if (backend.isTerminator(&second_last_op)) {
                    backend.last_inner_continue_block->back().erase();
                }
            }
        }
        backend.last_inner_continue_block = nullptr;
    }
    // check if we are nested inside another conditional or loop and if so:
    // branch to the appropriate block (depending on if its from a loop or cond)
    backend.setInsertionPointToStart(continue_block);
    if (!backend.nested_blocks_stack.empty()) {
        // check if block pair has a back_edge (loop)
        mlir::Block* block_to_branch_to;
        if (std::get<0>(backend.nested_blocks_stack.back()) != nullptr) {
            block_to_branch_to = std::get<0>(backend.nested_blocks_stack.back());
            mlir::Value old_sp = std::get<2>(backend.nested_blocks_stack.back());
            backend.emitRestoreStackPointer(old_sp);
        }
        else
            block_to_branch_to = std::get<1>(backend.nested_blocks_stack.back());
        backend.emitBranch(block_to_branch_to); // in case current continue block doesn't emit a terminal op
        backend.last_inner_continue_block = continue_block;
        // save this block in case we need to remove the branch instruction later
    }
    backend.setInsertionPointToStart(continue_block);
    backend.freeVector(domain_vector);
    return {};
}


std::any CodeGenASTWalker::visitCondNode(std::shared_ptr<CondNode> node) {
    // get the control expression value
    auto cond = std::any_cast<mlir::Value>(node->contr_expr->accept(*this));
    auto ip = backend.saveInsertionPoint();

    // then, else, continue block creation
    auto then_block = backend.createBlock();
    mlir::Block *else_block = nullptr;
    if (node->else_block)
        else_block = backend.createBlock();
    auto continue_block = backend.createBlock();

    // determine the control expression's second branch option
    backend.restoreInsertionPoint(ip);
    if (node->else_block)
        backend.emitCondBranch(cond, then_block, else_block);
    else
        backend.emitCondBranch(cond, then_block, continue_block);

    // populate the then block
    backend.setInsertionPointToStart(then_block);

    // push the parent continue block on the continue block stack
    backend.nested_blocks_stack.emplace_back(nullptr, continue_block, nullptr, nullptr);
    node->if_block->accept(*this);
    backend.nested_blocks_stack.pop_back();

    // add the branch to either the else block or the continue block.
    // If the last op in the then block is a terminator, we can't add a branch
    if (!backend.hasTerminator(then_block)) {
        backend.emitBranch(continue_block);
    }
    // populate the else block
    if (node->else_block) {
        backend.setInsertionPointToStart(else_block);
        backend.nested_blocks_stack.emplace_back(nullptr, continue_block, nullptr, nullptr);
        node->else_block->accept(*this);
        backend.nested_blocks_stack.pop_back();

        // check if the else branch already has a terminator
        if (!backend.hasTerminator(else_block))
            backend.emitBranch(continue_block);
    }
    // check the validity of the last inner continue block (eliminate terminal ops that aren't reached)
    if (backend.last_inner_continue_block != nullptr) {
        if (backend.last_inner_continue_block->getOperations().size() > 2) {
            // Erase the last 2 ops if third-to-last op is terminal.
            auto &third_last_op = *std::prev(backend.last_inner_continue_block->end(), 3);
            if (backend.isTerminator(&third_last_op)) {
                backend.last_inner_continue_block->back().erase();
                backend.last_inner_continue_block->back().erase();
            }
        }
        if (backend.last_inner_continue_block->getOperations().size() > 1) {
            // Erase the last op if second-to-last op is terminal.
            auto &second_last_op = *std::prev(backend.last_inner_continue_block->end(), 2);
            if (backend.isTerminator(&second_last_op)) {
                backend.last_inner_continue_block->back().erase();
            }
        }
        backend.last_inner_continue_block = nullptr;
    }
    // if the cond node is terminal, mark the continue block as unreachable
    backend.setInsertionPointToStart(continue_block);
    if (node->is_terminal)
        backend.emitUnreachableOp();

        // the continue block is reachable, check if we are nested inside another conditional or loop and if so:
        // branch to the appropriate block (depending on if its from a loop or cond)
    else if (!backend.nested_blocks_stack.empty()) {
        // check if block pair has a back_edge (loop)
        mlir::Block* block_to_branch_to;
        if (std::get<0>(backend.nested_blocks_stack.back()) != nullptr) {
            block_to_branch_to = std::get<0>(backend.nested_blocks_stack.back());
            mlir::Value old_sp = std::get<2>(backend.nested_blocks_stack.back());
            backend.emitRestoreStackPointer(old_sp);
        }
        else
            block_to_branch_to = std::get<1>(backend.nested_blocks_stack.back());
        backend.emitBranch(block_to_branch_to); // in case current continue block doesn't emit a terminal op
        backend.last_inner_continue_block = continue_block;
        // save this block in case we need to remove the branch instruction later
    }
    backend.setInsertionPointToStart(continue_block);
    return {};
}


std::any CodeGenASTWalker::visitOutStreamNode(const std::shared_ptr<OutStreamNode> node) {
    const auto result = std::any_cast<mlir::Value>(node->expr->accept(*this));
    if (std::dynamic_pointer_cast<VectorType>(node->type)) {
        backend.printVector(result, node->type);
        //auto vector_ptr = backend.getVector(result);
        backend.freeVector(result);
        return {};
    } else if (std::dynamic_pointer_cast<MatrixType>(node->type)){
      backend.emitPrintMatrix(result, node->type);
      backend.freeMatrix(result);
      return {};
    } else {
      backend.outStream(result);
      return {};
    }
}

std::any CodeGenASTWalker::visitStreamStateCallNode(std::shared_ptr<StreamStateCallNode> node) {
  return backend.loadStreamState();
}

std::any CodeGenASTWalker::visitInStreamNode(std::shared_ptr<InStreamNode> node) {
    Type::BaseType baseType = node->type->getBaseType();
    std::cout<<node->type->getTypeAsString()<<"\n";
    if (const auto idNode = std::dynamic_pointer_cast<IdNode>(node->l_value)) {
        auto symbol = node->containing_scope->resolve(idNode->id);
        const mlir::Value inStreamVal = backend.inStream(symbol->type);
        backend.emitAssignId(symbol, inStreamVal, node->line);
        return {};
    } if (const auto tupAccessNode = std::dynamic_pointer_cast<TupleAccessNode>(node->l_value)) {
        const auto symbol = node->containing_scope->resolve(tupAccessNode->tuple_id->id);
        int pos = 0;
        if (const auto intNode = std::dynamic_pointer_cast<IntNode>(tupAccessNode->element)) {
            pos = intNode->val - 1;
        } else if (const auto idNode = std::dynamic_pointer_cast<IdNode>(tupAccessNode->element)) {
            std::vector<std::string> elemNameVec = std::dynamic_pointer_cast<TupleType>(symbol->type)->element_names;
            if (const auto elemPos = find(elemNameVec.begin(), elemNameVec.end(), idNode->id);
                elemPos != elemNameVec.end()) {
                // Calculate the index and cast to int
                pos = static_cast<int>(elemPos - elemNameVec.begin());
            }
        }
        const mlir::Value inStreamVal = backend.inStream(
            std::dynamic_pointer_cast<TupleType>(symbol->type)->element_types[pos]);
        backend.emitTupAssign(symbol, pos, inStreamVal);
        return {};
    }if (const auto idxNode = std::dynamic_pointer_cast<IndexNode>(node->l_value)) {
        if (std::dynamic_pointer_cast<VectorType>(idxNode->collection->type)){
            mlir::Value array;
            std::shared_ptr<Type> type;
            if (auto id_node = std::dynamic_pointer_cast<IdNode>(idxNode->collection)) {
                auto array_sym = node->containing_scope->resolve(id_node->id);
                array = std::any_cast<mlir::Value>(array_sym->value);
                array = backend.getVector(array); //Load the array value from the alloca.
                type = array_sym->type;

            } else {
                throw SyntaxError(node->line, "Invalid assign of a vector");
            }
            auto index = std::any_cast<mlir::Value>(idxNode->index1->accept(*this));
            index = backend.emitSub(index, backend.emitInt(1));//Fix index
            PrimitiveType primitive;
            primitive.base_type = type->getBaseType();
            const mlir::Value inStreamVal = backend.inStream(std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive)));
            auto value = backend.typeCast(inStreamVal, type->getBaseType());
            backend.storeArrVal(array, index, value, type);
        }else if (std::dynamic_pointer_cast<MatrixType>(idxNode->collection->type)){
            mlir::Value matrix;
            std::shared_ptr<Type> matType;
            if (auto id_node = std::dynamic_pointer_cast<IdNode>(idxNode->collection)) {
                auto sym = node->containing_scope->resolve(id_node->id);
                matrix = std::any_cast<mlir::Value>(sym->value);
                matType = sym->type;
            } else {
                throw SyntaxError(node->line, "Invalid assign of a Matrix");
            }
            auto rowIntNode = std::dynamic_pointer_cast<IntNode>(idxNode->index1);
            auto row = rowIntNode->val-1;
            auto colIntNode = std::dynamic_pointer_cast<IntNode>(idxNode->index2);
            auto col = colIntNode->val-1;
            PrimitiveType primitive;
            primitive.base_type = matType->getBaseType();
            const mlir::Value inStreamVal = backend.inStream(std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive)));
            backend.setMatrixIndexValue(inStreamVal ,matrix, row, col, matType);
        }
    }else {
        throw TypeError(node->line, "Invalid assign Type");
    }
    return {};
}

std::any CodeGenASTWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
    auto func_symbol = node->containing_scope->resolve(node->func_id);
    bool is_prototype = true;
    if (node->func_block)
        is_prototype = false;

    // emit the funcOp and (set the insertion point to our entry block if function def is not a prototype)
    backend.emitFuncDefStart(func_symbol, is_prototype);

    // emit to the block, if the function has one...
    if (not is_prototype) {
        auto result = node->func_block->accept(*this);
    }
    // set the insertion point to the end of the module (finishing the function definition/prototype)
    backend.emitFuncDefEnd();
    return {};
}


std::any CodeGenASTWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
    auto proc_symbol = node->containing_scope->resolve(node->proc_id);
    bool is_prototype = true;

    if (node->proc_block)
        is_prototype = false;

    // emit the funcOp (set the insertion point to our entry block if procedure def is not a prototype)
    backend.emitProcDefStart(proc_symbol, is_prototype);

    // emit to the block, if the procedure has one...
    if (not is_prototype) {
        auto result = node->proc_block->accept(*this);
    }
    // set the insertion point to the end of the module (finishing the proc def/prototype)
    backend.emitProcDefEnd();
    return {};
}


std::any CodeGenASTWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {
    auto symbol = node->containing_scope->resolveProcFunc(node->id);
    std::vector<mlir::Value> eval_params;

    // emit the arguments to be passed to the proc/func call (if there are any)
    // store the mlir::Values (args) in the symbol table so that emitFuncProcCall can use them
    if (auto proc_symbol = std::dynamic_pointer_cast<ProcedureSymbol>(symbol)) {
        auto params = node->params;
        //for (const auto& param : params) {
        for (int i = 0; i < params.size(); i++) {
            // check if the parameter is mutable, and set flag if it is
            mlir::Value arg_value;
            if (auto id_param = std::dynamic_pointer_cast<IdNode>(params[i])) {
                passing_mutable_id = proc_symbol->orderedArgs[i]->mutability;
            }

            if (! proc_symbol->orderedArgs[i]->mutability && //If mutability is false, or in other words the symbol is a constant then
                std::dynamic_pointer_cast<VectorType>(proc_symbol->orderedArgs[i]->type)) {
                arg_value = std::any_cast<mlir::Value>(params[i]->accept(*this));
                arg_value = backend.emitVectorDec(arg_value); //This creates an alloca for the vector which we can pass into the next.
            } else {
                arg_value = std::any_cast<mlir::Value>(params[i]->accept(*this));
            }

            eval_params.push_back(arg_value);
            passing_mutable_id = false;
        }
        auto resultProc = backend.emitFuncProcCall(symbol, eval_params);

        // free dynamically allocated return value for discarded procedure calls
        if (node->call_used) {
            if (std::dynamic_pointer_cast<TupleType>(proc_symbol->type)) {
                //TODO: free tuple
            }
            else if (std::dynamic_pointer_cast<VectorType>(proc_symbol->type)) {
                backend.freeVector(resultProc);
            }
            else if (std::dynamic_pointer_cast<MatrixType>(proc_symbol->type)) {
                //TODO: free matrix
                backend.freeMatrix(resultProc);
            }
        }
        return resultProc;
    }
    else if (auto func_symbol = std::dynamic_pointer_cast<FunctionSymbol>(symbol)) {
        for (int i = 0; i < int(node->params.size()); i++) {
            auto arg_value = std::any_cast<mlir::Value>(node->params[i]->accept(*this));
            eval_params.push_back(arg_value);
        }
        auto resultFunc = backend.emitFuncProcCall(symbol, eval_params);

        return resultFunc;
    }
}


std::any CodeGenASTWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {

    mlir::Value expr_val;
    if (node->expr) {
        //expr_val will hold a pointer to a copy of any malloc'd objects, so we are safe to free after this
        expr_val = std::any_cast<mlir::Value>(node->expr->accept(*this));
    }
    //free everything that was malloc'd
    node->free_node->accept(*this);

    // check if there are any domain vectors that need to be freed
    for (auto block_stack_entry : backend.nested_blocks_stack) {
        auto domain_vec = std::get<3>(block_stack_entry);
        if (domain_vec != nullptr) {
            backend.freeVector(domain_vec);
        }
    }
    return backend.emitReturn(expr_val);
}


std::any CodeGenASTWalker::visitIdNode(const std::shared_ptr<IdNode> node) {
    const auto symbol = node->containing_scope->resolve(node->id);
    if (node->containing_scope->getScopeName() == "global") {
        return backend.emitLoadID(symbol, node->id);
    }
    // we are passing a mutable id to a procedure, we want to pass by reference instead of by value, return the pointer
    if (passing_mutable_id) {
        return symbol->value;
    }
    // pass by value
    return backend.emitLoadID(symbol);
}


std::any CodeGenASTWalker::visitBreakNode(std::shared_ptr<BreakNode> node) {
    // get the continue block of the innermost loop on the stack
    for (auto it = backend.nested_blocks_stack.rbegin(); it != backend.nested_blocks_stack.rend(); ++it) {
        if (std::get<0>(*it) != nullptr) {
            auto continue_block = std::get<1>(*it);
            auto old_sp = std::get<2>(*it);
            backend.emitRestoreStackPointer(old_sp);
            backend.emitBranch(continue_block);
            break;
        }
    }
    return {};
}

std::any CodeGenASTWalker::visitContinueNode(std::shared_ptr<ContinueNode> node) {
    // get the loop block of the innermost loop on the stack
    for (auto it = backend.nested_blocks_stack.rbegin(); it != backend.nested_blocks_stack.rend(); ++it) {
        if (std::get<0>(*it) != nullptr) {
            auto loop_block = std::get<0>(*it);
            auto old_sp = std::get<2>(*it);
            backend.emitRestoreStackPointer(old_sp);
            backend.emitBranch(loop_block);
            break;
        }
    }
    return {};
}


std::any CodeGenASTWalker::visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) {
    const auto symbol = node->containing_scope->resolve(node->tuple_id->id);
    // Get the pos of the element
    int pos = 0;
    if (const auto intNode = std::dynamic_pointer_cast<IntNode>(node->element)) {
        pos = intNode->val - 1;
    } else if (const auto idNode = std::dynamic_pointer_cast<IdNode>(node->element)) {
        std::vector<std::string> elemNameVec = std::dynamic_pointer_cast<TupleType>(symbol->type)->element_names;
        const auto elemPos = find(elemNameVec.begin(), elemNameVec.end(), idNode->id);
        if (elemPos != elemNameVec.end()) {
            // Calculate the index and cast to int
            pos = static_cast<int>(elemPos - elemNameVec.begin());
        }
    } else {
        std::cout << "If it came here then there was a problem" << std::endl;
    }
    return backend.emitTupleAccess(symbol, pos);
}

std::any CodeGenASTWalker::visitRowsCallNode(std::shared_ptr<RowsCallNode> node) {
  auto result = std::any_cast<mlir::Value>(node->matrix->accept(*this));
  auto rows = backend.emitGetRows(result);
  backend.freeMatrix(result);
  return rows;
}
std::any CodeGenASTWalker::visitColumnsCallNode(std::shared_ptr<ColumnsCallNode> node){
  auto result = std::any_cast<mlir::Value>(node->matrix->accept(*this));
  auto cols = backend.emitGetCols(result);
  backend.freeMatrix(result);
  return cols;
}


std::any CodeGenASTWalker::visitFreeNode(std::shared_ptr<FreeNode> node) {

    // pass through the scope's symbol table and free everything that is malloc'd
    auto scope = std::dynamic_pointer_cast<BaseScope>(node->scope_to_free_up_to);
    for (const auto& symbol : scope->symbols_to_free) {
        if (auto tuple_type = std::dynamic_pointer_cast<TupleType>(symbol->type)) {
            // FREE TUPLE
            int i = 0;
            for (auto elem_type : tuple_type->element_types) {
                if (std::dynamic_pointer_cast<VectorType>(elem_type)) {
                    //FREE VECTOR ELEMENT
                    auto tuple_vector_ptr = backend.getTupleElemPointer(symbol, i);
                    auto vector_ptr = backend.getVector(tuple_vector_ptr);
                    backend.freeVector(vector_ptr);
                }
                else if (std::dynamic_pointer_cast<MatrixType>(elem_type)) {
                    //FREE MATRIX ELEMENT
                    auto matrix_ptr = backend.getTupleElemPointer(symbol, i);
                    backend.freeMatrix(matrix_ptr);
                }
                i++;
            }
        }
        else if (std::dynamic_pointer_cast<VectorType>(symbol->type)) {
            // free vector
            auto vector_ptr = backend.getVector(std::any_cast<mlir::Value>(symbol->value));
            backend.freeVector(vector_ptr);
        }
        else if (std::dynamic_pointer_cast<MatrixType>(symbol->type)) {
            //FREE MATRIX
            auto matrix_ptr = std::any_cast<mlir::Value>(symbol->value);
            backend.freeMatrix(matrix_ptr);
        }
    }
    return {};
}


std::any CodeGenASTWalker::visitReverseCallNode(std::shared_ptr<ReverseCallNode> node) {
    const auto vector = std::any_cast<mlir::Value>(node->vector->accept(*this));
    return backend.reverseVector(vector,node->type);
}

std::any CodeGenASTWalker::visitLengthCallNode(std::shared_ptr<LengthCallNode> node) {
    const auto vector = std::any_cast<mlir::Value>(node->vector->accept(*this));
    return backend.getArrSize(vector,true);
}
std::any CodeGenASTWalker::visitFormatCallNode(std::shared_ptr<FormatCallNode> node) {
    const auto scalar = std::any_cast<mlir::Value>(node->scalar->accept(*this));
    return backend.emitFormatScalar(scalar);
}