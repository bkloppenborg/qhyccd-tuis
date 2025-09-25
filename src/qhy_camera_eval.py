#!/usr/bin/env python3
"""
QHY Camera Performance Evaluation

This script automates data collection from a QHY camera across multiple readout modes,
filters, and exposure times. It systematically captures images with different
configurations for camera performance characterization and analysis.

Usage:
    python3 qhy_camera_eval.py --camera-id <camera_id> [options]

Requirements:
    - qhy-camera-control binary built and accessible
    - Camera configuration files (optional)
    - Appropriate filter wheel setup
"""

import argparse
import subprocess
import sys
import os
import time
import logging
import numpy as np
from pathlib import Path

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('qhy_camera_eval.log'),
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger(__name__)

class QHYCameraEvaluator:
    def __init__(self, args):
        self.args = args
        self.qhy_binary = self._find_qhy_binary()
        self.base_config = self._build_base_config()

    def _find_qhy_binary(self):
        """Find the qhy-camera-control binary"""
        possible_paths = [
            '../build/src/qhy-camera-control',
            './build/src/qhy-camera-control',
            'qhy-camera-control'
        ]

        for path in possible_paths:
            if os.path.exists(path) or subprocess.run(['which', path],
                                                    capture_output=True).returncode == 0:
                logger.info(f"Found qhy-camera-control binary at: {path}")
                return path

        raise FileNotFoundError("Could not find qhy-camera-control binary")

    def _build_base_config(self):
        """Build base configuration arguments"""
        config = [
            '--no-gui',
            '--camera-id', self.args.camera_id,
            '--catalog', self.args.catalog,
            '--object-id', self.args.object_id
        ]

        # Add configuration file if specified
        if self.args.config_file:
            config.extend(['--config-file', self.args.config_file])

        # Add camera config block if specified
        if self.args.camera_config:
            config.extend(['--camera-config', self.args.camera_config])

        # Add site config block if specified
        if self.args.site_config:
            config.extend(['--site-config', self.args.site_config])

        # Add exposure config block if specified
        if self.args.exp_config:
            config.extend(['--exp-config', self.args.exp_config])

        # Add save directory
        if self.args.save_dir:
            config.extend(['--save-dir', self.args.save_dir])

        return config

    def _generate_exposure_times(self):
        """Generate exposure times: 0.01, 0.03, 0.1, 0.3, 1, 3, 10, 30, 100"""
        # Pattern: multiply by 3, then by ~3.33 to get next decade
        # 0.01 -> 0.03 -> 0.1 -> 0.3 -> 1 -> 3 -> 10 -> 30 -> 100
        base_sequence = [0.01, 0.03, 0.1]
        exposure_times = []

        # Generate the sequence across decades
        for decade in [0.01, 0.1, 1.0, 10.0]:
            for multiplier in [1, 3, 10]:
                value = decade * multiplier
                if value <= 100.0:
                    exposure_times.append(value)

        # Remove duplicates and sort
        exposure_times = sorted(list(set(exposure_times)))

        logger.info(f"Generated {len(exposure_times)} exposure times: {exposure_times}")
        return exposure_times

    def capture_images(self, read_mode, filter_name, exposure_time, num_exposures=5, dry_run=False):
        """Capture images for a single configuration"""
        if dry_run:
            logger.info(f"DRY RUN - Would capture: Read Mode {read_mode}, Filter {filter_name}, "
                       f"Exposure {exposure_time}s, Count {num_exposures}")
        else:
            logger.info(f"Capturing: Read Mode {read_mode}, Filter {filter_name}, "
                       f"Exposure {exposure_time}s, Count {num_exposures}")

        # Build command
        cmd = [self.qhy_binary] + self.base_config + [
            '--camera-read-mode', str(read_mode),
            '--exp-filters', filter_name,
            '--exp-durations', str(exposure_time),
            '--exp-quantities', str(num_exposures),
            '--exp-gains', '1.0',
            '--exp-offsets', '30'
        ]

        # Add filter names if provided
        if self.args.filter_names:
            cmd.extend(['--filter-names', self.args.filter_names])

        if dry_run:
            # In dry run mode, just print the command
            cmd_str = ' '.join(f'"{arg}"' if ' ' in arg else arg for arg in cmd)
            logger.info(f"Command: {cmd_str}")
            return True

        try:
            logger.debug(f"Executing command: {' '.join(cmd)}")
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)

            if result.returncode != 0:
                logger.error(f"Capture failed with return code {result.returncode}")
                logger.error(f"STDOUT: {result.stdout}")
                logger.error(f"STDERR: {result.stderr}")
                return False
            else:
                logger.debug(f"Capture completed successfully")
                return True

        except subprocess.TimeoutExpired:
            logger.error(f"Capture timed out after 300 seconds")
            return False
        except Exception as e:
            logger.error(f"Capture failed with exception: {e}")
            return False

    def run_evaluation(self):
        """Run the complete camera evaluation"""
        read_modes = [0, 1]
        filters = ['Dark', 'SU', 'SG', 'SR', 'SI', 'SZ_S']
        exposure_times = self._generate_exposure_times()

        total_configurations = len(read_modes) * len(filters) * len(exposure_times)
        completed_configurations = 0
        skipped_configurations = 0

        logger.info(f"Starting camera evaluation: {total_configurations} total configurations")
        logger.info(f"Read modes: {read_modes}")
        logger.info(f"Filters: {filters}")
        logger.info(f"Exposure times: {len(exposure_times)} values from {min(exposure_times)} to {max(exposure_times)} seconds")

        start_time = time.time()

        for read_mode in read_modes:
            for filter_name in filters:
                for exposure_time in exposure_times:
                    success = self.capture_images(
                        read_mode=read_mode,
                        filter_name=filter_name,
                        exposure_time=exposure_time,
                        num_exposures=5,
                        dry_run=self.args.dry_run
                    )

                    completed_configurations += 1
                    if not success:
                        skipped_configurations += 1
                        if not self.args.dry_run:
                            logger.warning(f"Skipped configuration due to capture issue")

                    # Progress update
                    progress = (completed_configurations / total_configurations) * 100
                    elapsed = time.time() - start_time
                    eta = (elapsed / completed_configurations) * (total_configurations - completed_configurations)

                    if self.args.dry_run:
                        logger.info(f"Progress: {completed_configurations}/{total_configurations} ({progress:.1f}%)")
                    else:
                        logger.info(f"Progress: {completed_configurations}/{total_configurations} ({progress:.1f}%) "
                                  f"- Skipped: {skipped_configurations} - ETA: {eta/60:.1f} minutes")

                    # Small delay between configurations (skip in dry run)
                    if not self.args.dry_run and self.args.delay > 0:
                        time.sleep(self.args.delay)

        # Final summary
        total_time = time.time() - start_time
        successful_configurations = completed_configurations - skipped_configurations

        if self.args.dry_run:
            logger.info(f"DRY RUN completed in {total_time:.1f} seconds")
            logger.info(f"Total configurations: {total_configurations}")
            logger.info(f"Commands generated: {successful_configurations}")
        else:
            logger.info(f"Camera evaluation completed in {total_time/60:.1f} minutes")
            logger.info(f"Total configurations: {total_configurations}")
            logger.info(f"Successful captures: {successful_configurations}")
            logger.info(f"Skipped configurations: {skipped_configurations}")

        return successful_configurations > 0

def main():
    parser = argparse.ArgumentParser(
        description='QHY Camera Performance Evaluation - Automated data collection',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python3 qhy_camera_eval.py --camera-id QHY5III178M

    python3 qhy_camera_eval.py --camera-id QHY5III178M \\
        --catalog "NGC" --object-id "M31_EVAL" \\
        --config-file ../config/camera.ini \\
        --camera-config "main_camera" \\
        --save-dir ../evaluation_data \\
        --filter-names "Dark,SU,SG,SR,SI,SZ_S"
        """
    )

    # Required arguments
    parser.add_argument('--camera-id', required=True,
                       help='QHY Camera Identifier (e.g., QHY5III178M)')

    # Object identification arguments
    parser.add_argument('--catalog', default='TEST',
                       help='Catalog name for FITS headers (default: TEST)')
    parser.add_argument('--object-id', default='CHARACTERIZATION',
                       help='Object identifier for FITS headers (default: CHARACTERIZATION)')

    # Configuration arguments
    parser.add_argument('--config-file',
                       help='Path to configuration file')
    parser.add_argument('--camera-config',
                       help='Camera configuration block name')
    parser.add_argument('--site-config',
                       help='Site configuration block name')
    parser.add_argument('--exp-config',
                       help='Exposure configuration block name')

    # Optional arguments
    parser.add_argument('--save-dir', default='./evaluation_data',
                       help='Directory to save captured images (default: ./evaluation_data)')
    parser.add_argument('--filter-names',
                       help='Comma-separated list of filter names in wheel order')
    parser.add_argument('--delay', type=float, default=1.0,
                       help='Delay between captures in seconds (default: 1.0)')
    parser.add_argument('--dry-run', action='store_true',
                       help='Print commands without executing them')
    parser.add_argument('--verbose', action='store_true',
                       help='Enable verbose logging')

    args = parser.parse_args()

    # Set logging level
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    # Create save directory
    os.makedirs(args.save_dir, exist_ok=True)

    try:
        evaluator = QHYCameraEvaluator(args)

        if args.dry_run:
            logger.info("DRY RUN MODE - Commands will be printed but not executed")

        success = evaluator.run_evaluation()
        return 0 if success else 1

    except Exception as e:
        logger.error(f"Camera evaluation failed with error: {e}")
        return 1

if __name__ == '__main__':
    sys.exit(main())