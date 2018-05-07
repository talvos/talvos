// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

/// \file ComputePipeline.h
/// This file declares the ComputePipeline class.

#ifndef TALVOS_COMPUTEPIPELINE_H
#define TALVOS_COMPUTEPIPELINE_H

namespace talvos
{

class PipelineStage;

/// This class encapsulates a compute pipeline.
class ComputePipeline
{
public:
  /// Create a compute pipeline from a single pipeline stage.
  /// Ownership of \p Stage is transferred to the pipeline.
  ComputePipeline(PipelineStage *Stage) : Stage(Stage){};

  /// Destroy the pipeline.
  ~ComputePipeline();

  // Do not allow ComputePipeline objects to be copied.
  ///\{
  ComputePipeline(const ComputePipeline &) = delete;
  ComputePipeline &operator=(const ComputePipeline &) = delete;
  ///\}

  /// Returns the pipeline stage.
  const PipelineStage *getStage() const { return Stage; }

private:
  /// The pipeline stage in this pipeline.
  PipelineStage *Stage;
};

} // namespace talvos

#endif
