# include <Siv3D.hpp>
# include <HamFramework.hpp>

struct Smoke : IEffect
{
	Vec2 m_pos, m_speed;

	double m_scale;

	Smoke(const Vec2& pos, Vec2 dir, double scale)
		: m_pos(pos)
		, m_speed(dir.setLength(20).rotated(Random(-0.3, 0.3) * scale))
		, m_scale(scale) {}

	bool update(double t) override
	{
		TextureAsset(L"Particle").scale((0.5 + 4 * t) * m_scale * 0.1).drawAt(m_pos + m_speed * t, AlphaF(Pow(1.0 - t / 0.4, 4)));
		return t < 0.4;
	}
};

void Main()
{
	const ColorF skyColorTop(0.08, 0.04, 0.08), skyColorBottom(0.35, 0.25, 0.35);
	Graphics::SetBackground(skyColorTop);
	Window::Resize(1280, 720);
	ScalableWindow::Setup(1280, 720);

	TextureAsset::Register(L"Particle", L"Example/Particle.png", TextureDesc::Mipped);
	Effect effect;

	Array<Circle> stars;
	for (int32 i = 0; i < 200; ++i)
	{
		stars.emplace_back(RandomVec2(Rect(-80, -1, 160, 80)), Random(0.05, 0.2));
	}

	PerlinNoise noise;
	Array<Vec2> groundPoints;
	for (int32 i = -220; i <= 220; ++i)
	{
		const double h = Abs(i) < 6 ? 0.0 : noise.octaveNoise(i / 8.0, 4);
		groundPoints.emplace_back(i, h);
	}

	PhysicsWorld world(Vec2(0, -0.5));
	auto ground = world.createLineString(Vec2(0, 0), groundPoints, none, none, PhysicsBodyType::Static);

	std::reverse(groundPoints.begin(), groundPoints.end());
	groundPoints.emplace_back(-220, -20);
	groundPoints.emplace_back(220, -20);
	const Polygon groundPolygon(groundPoints);

	const Polygon polygon({ { -0.8, 2 },{ -1.6, -0.2 },{ -1.6, -1.0 },
	{ -2.2, -0.2 },{ -2.8, -3 },{ -2.6, -3.4 },{ -2.0, -1 },
	{ -1.6, -1.4 },{ -1.6, -2 },{ 1.6, -2 },{ 1.6, -1.4 },
	{ 2.0, -1 },{ 2.6, -3.4 },{ 2.8, -3 },{ 2.2, -0.2 },
	{ 1.6, -1.0 },{ 1.6, -0.2 },{ 0.8, 2 }
	});

	PhysicsBody rocket = world.createPolygon(Vec2(-8, 14), polygon)
		.addCircle(Circle(-2.7, -3.2, 0.4))
		.addCircle(Circle(2.7, -3.2, 0.4));
	CameraBox2D camera;
	CameraBox2D cameraStars;

	const Font fontTitle(90, Typeface::Thin),fontDistance(50, Typeface::Light), fontAngle(40, Typeface::Light);
	Vec2 previousSpeed;
	double damage = 0.0, fuel = 100.0;
	bool onLine = false, launched = false;
	
	while (System::Update())
	{
		if (System::FrameCount() == 1 || Input::KeyEnter.clicked)
		{
			rocket.setPos(-160, Random(46.0, 54.0));
			rocket.setAngle(Random(-30_deg, 30_deg));
			rocket.setAngularVelocity(0.0);
			rocket.setVelocity(previousSpeed = Vec2(Random(6.0, 6.6), 0));
			camera = CameraBox2D(rocket.getPos(), 8.0);
			cameraStars = CameraBox2D();
			fuel = 100.0;
			damage = 0.0;
			onLine = launched = false;
		}

		const Vec2 rocketPos = rocket.getPos();
		const double rocketAngle = rocket.getAngle();

		if (fuel > 0.0 || rocketPos.y > 100)
		{
			if (Input::KeyQ.pressed)
			{
				const Vec2 pos = rocketPos + Vec2(-0.8, 2).rotate(rocketAngle);
				const Vec2 dir = Vec2(2, -0.5).rotate(rocketAngle);
				rocket.applyForceAt(Vec2(2, -0.5).rotate(rocketAngle), pos);
				effect.add<Smoke>(pos, -dir, 0.5);
				fuel = Max(fuel - 0.005, 0.0);
			}

			if (Input::KeyZ.pressed)
			{
				const Vec2 pos = rocketPos + Vec2(-1.6, -2).rotate(rocketAngle);
				const Vec2 dir = Vec2(2, -0.5).rotate(rocketAngle);
				rocket.applyForceAt(Vec2(2, -0.5).rotate(rocketAngle), pos);
				effect.add<Smoke>(pos, -dir, 0.5);
				fuel = Max(fuel - 0.005, 0.0);
			}

			if (Input::KeySpace.pressed)
			{
				const Vec2 pos = rocketPos - Vec2(0, 2).rotate(rocketAngle);
				const Vec2 dir = Vec2(0, 2).rotate(rocketAngle);
				rocket.applyForce(Vec2(0, 20).rotate(rocketAngle));
				effect.add<Smoke>(pos, -dir, 1.0);
				fuel = Max(fuel - 0.04, 0.0);
			}

			if (Input::KeyM.pressed)
			{
				const Vec2 pos = rocketPos + Vec2(1.6, -2).rotate(rocketAngle);
				const Vec2 dir = Vec2(-2, -0.5).rotate(rocketAngle);
				rocket.applyForceAt(Vec2(-2, -0.5).rotate(rocketAngle), pos);
				effect.add<Smoke>(pos, -dir, 0.5);
				fuel = Max(fuel - 0.005, 0.0);
			}

			if (Input::KeyO.pressed)
			{
				const Vec2 pos = rocketPos + Vec2(0.8, 2).rotate(rocketAngle);
				const Vec2 dir = Vec2(-2, -0.5).rotate(rocketAngle);
				rocket.applyForceAt(Vec2(-2, -0.5).rotate(rocketAngle), pos);
				effect.add<Smoke>(pos, -dir, 0.5);
				fuel = Max(fuel - 0.005, 0.0);
			}
		}

		world.update();

		// ダメージ計算
		const double delta = rocket.getVelocity().distanceFrom(std::exchange(previousSpeed, rocket.getVelocity()));
		if (delta >= 0.05)
		{
			const double a = Abs(rocketAngle);
			damage = Min(damage + delta * (a < 2_deg ? 0.75 : a < 30_deg ? 1.0 : a < 50_deg ? 2.0 : 5.0), 99.9);
		}

		camera.setPos(Vec2(rocketPos.x + 20, 2 + Max(14.0, rocketPos.y * 0.55)));
		camera.setScale(16.0 - Min(rocketPos.y * 0.1, 8.0));
		cameraStars.setPos(Vec2(rocketPos.x * 0.02, 37));
		cameraStars.setScale(10);

		{
			const auto t1 = cameraStars.createTransformer();
			const auto t2 = ScalableWindow::CreateTransformer();

			// 宇宙
			Rect(-200, -4, 400, 24).draw({ skyColorBottom, skyColorBottom, skyColorTop, skyColorTop });

			// 星
			for (const auto& star : stars)
			{
				star.draw();
			}
		}

		{
			const auto t1 = camera.createTransformer();
			const auto t2 = ScalableWindow::CreateTransformer();

			const Polygon rocketPolygon = rocket.shapeAs<B2Polygon>(0)->getPolygon();

			// 距離ライン
			for (int32 i = -3; i <= 3; ++i)
			{
				const double a = (0.75 - Pow(Abs(i) * 0.3, 0.4) * 0.75) * Min(1.0, 4.0 - rocketPos.y * 0.05);
				const Line line(i * 20, -400, i * 20, 400);

				if (i == 0)
				{
					onLine = line.intersects(rocketPolygon);
					line.draw(0.8, onLine ? HSV(System::FrameCount(), 0.5, 1).toColorF(a) : AlphaF(a));
				}
				else
				{
					line.draw(0.8, AlphaF(a));
				}
			}

			// 月面
			ground.draw();
			groundPolygon.draw(Color(200, 200, 160));

			// 機体
			effect.update();
			rocketPolygon.drawFrame(0.2, Color(200, 220, 250));
			rocket.draw(Lerp(Palette::Skyblue, Color(60), damage/100.0));
		}

		{
			const auto t2 = ScalableWindow::CreateTransformer();
			fontDistance(L"{:.1f}m"_fmt, rocketPos.x * -4).draw(780, 40, Alpha(80));
			fontAngle(L"{:.1f}°"_fmt, Degrees(-rocketAngle)).draw(1080, 55, Alpha(80));
			fontAngle(L"FUEL: {:.1f}%"_fmt, fuel).draw(780, 140, Alpha(80));
			fontAngle(L"DAMAGE: {:.1f}%"_fmt, damage).draw(780, 200, Alpha(80));
		}

		if (onLine && rocketPos.y > 100)
		{
			launched = true;
		}

		if(launched)
		{
			const double alpha = Min((rocketPos.y - 100.0) / 10.0, 1.0) * 0.75;
			fontTitle(L"Lunar").drawCenter(Window::BaseCenter().x, Window::BaseCenter().y + 40 , ColorF(1, 1, 0, alpha));
			fontTitle(L"Excursion").drawCenter(Window::BaseCenter().x, Window::BaseCenter().y + 200, ColorF(1, 1, 0, alpha));
		}

		ScalableWindow::DrawBlackBars();
	}
}
