// trainInputMatrices.cpp

/** @file
    Naïve Supervised Networks Trainer.

    @author Nicolas Chaussé

    @copyright Copyright 2022 Nicolas Chaussé (nicolaschausse@protonmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License only.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    @version 0.1

    @date 2022

    @todo Select matrix digraph and weights crafter on the command line.
    @todo Explore sticky tasks to gofer threads.
    @todo Explore compute weights asynchronously.
*/

/*
**************
** INCLUDES **
**************
*/

#include <csignal>
#include <cstdlib>
#include <exception>
#include <memory>

#include "NaiveSupervisedNetworks.hpp"

/*
**********************
** GLOBAL VARIABLES **
**********************
*/

// Global (static) raw pointer as signal lambdas can not capture.
static SupervisedNetworkTrainer* GlobalSupervisedNetworkTrainer;

/*
**********
** MAIN **
**********
*/

int
main(int const argumentsCount, char const* const* const arguments)
{
  int exitStatus{ EXIT_FAILURE };

  // Try to create the logger.
  try {
    Logger logger("TRAIN");

    // Try to build the SupervisedNetworkTrainer and run it.
    try {
      // When there will be more than one matrix digraph type, they may be selected at run time on the command line.
      SupervisedNetworkTrainer::MatrixDigraphsMap const matrixDigraphsMap{
        { "LogarithmicMatrixDigraph",
          [](auto rowsCount, auto columnsCount) {
            return std::make_unique<LogarithmicMatrixDigraph>(rowsCount, columnsCount);
          } }
      };

      // When there will be more then one weights crafter type, they may be selected at run time on the command line.
      SupervisedNetworkTrainer::WeightsCraftersMap const weightsCraftersMap{
        { "GeometricWeightsCrafter",
          [](auto weightsCount) { return std::make_shared<GeometricWeightsCrafter>(weightsCount); } }
      };

      logger.banner() << "Building the supervised network trainer...\n\n";
      SupervisedNetworkTrainer supervisedNetworkTrainer;
      if (supervisedNetworkTrainer.populateFromArguments(
            logger, argumentsCount, arguments, matrixDigraphsMap, weightsCraftersMap)) {
        struct Deallocator
        {
          // This destructor is guaranteed to be called on leaving the present context, even on caught exceptions.
          ~Deallocator() noexcept
          {
            // Set the stop signal handlers back to defaults (and silence warning cert-err33-c).
            static_cast<void>(std::signal(SIGABRT, SIG_DFL));
            static_cast<void>(std::signal(SIGINT, SIG_DFL));
            static_cast<void>(std::signal(SIGTERM, SIG_DFL));
            GlobalSupervisedNetworkTrainer = nullptr;
          }
        } deallocator;

        /** Set the stop signal handlers to request supervisedNetworkTrainer to die gracefully.
            Needs global (static) GlobalSupervisedNetworkTrainer raw pointer as signal lambdas can not capture.
        */
        GlobalSupervisedNetworkTrainer = &supervisedNetworkTrainer;
        auto const exitSignalsHandler{ +[](int const) { GlobalSupervisedNetworkTrainer->stop(); } };
        if (SIG_ERR == std::signal(SIGABRT, exitSignalsHandler))
          throw std::runtime_error(String(+"Can not set handler for signal SIGABRT in: ", +__PRETTY_FUNCTION__, '.'));
        if (SIG_ERR == std::signal(SIGINT, exitSignalsHandler))
          throw std::runtime_error(String(+"Can not set handler for signal SIGINT in: ", +__PRETTY_FUNCTION__, '.'));
        if (SIG_ERR == std::signal(SIGTERM, exitSignalsHandler))
          throw std::runtime_error(String(+"Can not set handler for signal SIGTERM in: ", +__PRETTY_FUNCTION__, '.'));

        // Run!
        logger.banner() << "Running the supervised network trainer...\n\n\t███  PRESS Ctrl-C TO STOP!  ███\n";
        supervisedNetworkTrainer.run(logger);

        exitStatus = EXIT_SUCCESS;
      }
    } catch (std::exception const& exception) {
      logger << "\n██ FATAL EXCEPTION " << TypeNameOf(exception) << ":\n" << exception.what() << "\n\n";
    } catch (...) {
      logger << "\n██ UNKNOWN FATAL EXCEPTION!\n\n";
    }

    logger.banner() << "DONE.\n\n";

  } catch (std::exception const& exception) {
    std::cerr << "\nFATAL EXCEPTION " << TypeNameOf(exception) << ", can not instantiate the logger:\n"
              << exception.what() << "\n\n";
  } catch (...) {
    std::cerr << "\nUNKNOWN FATAL EXCEPTION! Can not instantiate the logger.\n\n";
  }

  return exitStatus;
}
