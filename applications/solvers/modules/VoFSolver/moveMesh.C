/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2022-2023 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "VoFSolver.H"
#include "CorrectPhi.H"
#include "geometricZeroField.H"

// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

void Foam::solvers::VoFSolver::moveMesh()
{
    if (pimple.firstIter() || pimple.moveMeshOuterCorrectors())
    {
        if
        (
            correctPhi
         && divergent()
         && !divU.valid()
        )
        {
            // Construct and register divU for correctPhi
            divU = new volScalarField
            (
                "divU0",
                fvc::div(fvc::absolute(phi, U))
            );
        }

        // Move the mesh
        mesh.move();

        if (mesh.changing())
        {
            buoyancy.moveMesh();

            MRF.update();

            if (correctPhi || mesh.topoChanged())
            {
                // Calculate absolute flux
                // from the mapped surface velocity
                phi = mesh.Sf() & Uf();

                correctUphiBCs(U, phi, true);

                if (correctPhi)
                {
                    if (divU.valid())
                    {
                        CorrectPhi
                        (
                            phi,
                            U,
                            p_rgh,
                            surfaceScalarField("rAUf", fvc::interpolate(rAU())),
                            divU(),
                            pressureReference(),
                            pimple
                        );
                    }
                    else
                    {
                        CorrectPhi
                        (
                            phi,
                            U,
                            p_rgh,
                            surfaceScalarField("rAUf", fvc::interpolate(rAU())),
                            geometricZeroField(),
                            pressureReference(),
                            pimple
                        );
                    }
                }

                // Make the fluxes relative to the mesh motion
                fvc::makeRelative(phi, U);
            }

            meshCourantNo();

            correctInterface();
        }

        divU.clear();
    }
}


// ************************************************************************* //
