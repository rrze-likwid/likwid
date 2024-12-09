/*
 * =======================================================================================
 *
 *      Filename:  frequency_pstate.h
 *
 *      Description:  Header File frequency module, the Intel PState backend
 *
 *      Version:   5.4.1
 *      Released:  09.12.2024
 *
 *      Author:   Thomas Gruber (tr), thomas.roehl@googlemail.com
 *      Project:  likwid
 *
 *      Copyright (C) 2024 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 3 of the License, or (at your option) any later
 *      version.
 *
 *      This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *      WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *      PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along with
 *      this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =======================================================================================
 */

#ifndef LIKWID_FREQUENCY_PSTATE
#define LIKWID_FREQUENCY_PSTATE

uint64_t freq_pstate_getCpuClockMax(const int cpu_id );
uint64_t freq_pstate_getCpuClockMin(const int cpu_id );

int freq_pstate_getTurbo(const int cpu_id );

#endif
