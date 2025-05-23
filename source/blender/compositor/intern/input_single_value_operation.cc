/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "BLI_assert.h"
#include "BLI_math_vector_types.hh"

#include "COM_input_single_value_operation.hh"
#include "COM_operation.hh"
#include "COM_result.hh"
#include "COM_utilities.hh"

namespace blender::compositor {

const StringRef InputSingleValueOperation::output_identifier_ = StringRef("Output");

InputSingleValueOperation::InputSingleValueOperation(Context &context, DInputSocket input_socket)
    : Operation(context), input_socket_(input_socket)
{
  const ResultType result_type = get_node_socket_result_type(input_socket_.bsocket());
  Result result = context.create_result(result_type);

  /* The result of an input single value operation is guaranteed to have a single user. */
  result.set_initial_reference_count(1);

  populate_result(result);
}

void InputSingleValueOperation::execute()
{
  /* Allocate a single value for the result. */
  Result &result = get_result();
  result.allocate_single_value();

  const bNodeSocket *bsocket = input_socket_.bsocket();

  /* Set the value of the result to the default value of the input socket. */
  switch (result.type()) {
    case ResultType::Float:
      result.set_single_value(bsocket->default_value_typed<bNodeSocketValueFloat>()->value);
      break;
    case ResultType::Int:
      result.set_single_value(bsocket->default_value_typed<bNodeSocketValueInt>()->value);
      break;
    case ResultType::Vector:
      result.set_single_value(
          float4(float3(bsocket->default_value_typed<bNodeSocketValueVector>()->value), 0.0f));
      break;
    case ResultType::Color:
      result.set_single_value(float4(bsocket->default_value_typed<bNodeSocketValueRGBA>()->value));
      break;
    case ResultType::Float2:
    case ResultType::Float3:
    case ResultType::Int2:
      /* Those types are internal and needn't be handled by operations. */
      BLI_assert_unreachable();
      break;
  }
}

Result &InputSingleValueOperation::get_result()
{
  return Operation::get_result(output_identifier_);
}

void InputSingleValueOperation::populate_result(Result result)
{
  Operation::populate_result(output_identifier_, result);
}

}  // namespace blender::compositor
