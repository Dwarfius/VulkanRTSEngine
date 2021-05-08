{
	"myUID": {
		"myMac": 0,
		"myTime": 0,
		"myRndNum": 0
	},
	"myLocalTransf": {
		"myPos": [0, 0, 0],
		"myScale": [1, 1, 1],
		"myRotation": [0, 0, 0, 1]
	},
	"myCenter": [0, 0, 0],
	"myComponents": [
		{
			"myCompType": "PhysicsComponent",
			"myCompData": {
				"myMass": 0,
				"myShape": 1,
				"myHalfExtents": [0.5, 0.5, 0.5],
				"myOrigin": [0, 0, 0]
			}
		},
        {
            "myCompType": "VisualComponent",
            "myCompData": {
                "myTransf": {
                    "myPos": [0, 0, 0],
                    "myScale": [1, 1, 1],
                    "myRotation": [0, 0, 0, 1]
                },
                "myPipeline": "Engine/default.ppl",
                "myTextures": [
                    "CubeUnwrap.img"
                ],
                "myModel": "TestGameObject/modelTest.bin"
            }
        }
	],
	"myChildren": []
}